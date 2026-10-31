// Room.Render的输出仍由本地引擎产生；只补齐Game.Render中房间必需的原生颜色后处理。
// 不接收Authority像素、最终顶点或绘制批次，也不在这里重画Room/Grid。
void ColorTarget(Scene &s){
 GLint viewport[4]{};glGetIntegerv(GL_VIEWPORT,viewport);
 if(viewport[2]<=0||viewport[3]<=0||std::uint64_t(viewport[2])*unsigned(viewport[3])>8u*1024u*1024u)Fail("native_color_dimensions");
 if(s.color_target.image)return;
 static std::uint64_t serial=0;KColor transparent(0,0,0,0);
 const auto name="isaac.cs.native-room-color."+std::to_string(++serial);
 s.color_target=KAGE_Graphics_ImageManager::CreateProceduralImage(unsigned(viewport[2]),unsigned(viewport[3]),name.c_str(),transparent);
 if(!s.color_target.image)Fail("native_color_target");
}
void RenderRoom(Scene &s){
 auto &manager=g_KAGE_Graphics_Manager;auto *old_target=manager.currentRenderTarget;const auto old_screen=manager.renderTargetUsesScreenSize;
 auto *old_shader=manager._shader;const auto old_blend=manager._blendMode;bool offscreen=false;
 auto restore=[&](){if(offscreen){manager.Present();manager.SetCurrentRenderTarget(old_target,old_screen);}manager._shader=old_shader;manager._blendMode=old_blend;};
 try{
  manager.SetCurrentRenderTarget(s.color_target.image,true);offscreen=true;manager.Clear();
  Lua("Render",s.room,lua::Metatables::ROOM);manager.Present();manager.SetCurrentRenderTarget(old_target,old_screen);offscreen=false;
  auto *shader=__ptr_g_AllShaders[SHADER_COLOR_MOD];if(!shader||!shader->_initialized)Fail("native_color_shader");
  std::vector<std::uint8_t> formats;unsigned stride=0;
  for(unsigned i=0;i<shader->_numVertexAttributes;++i){const auto f=shader->_vertexAttributes[i].format;unsigned count=0;
   switch(f){case 1:case 8:count=1;break;case 2:case 7:count=2;break;case 3:case 5:count=3;break;case 4:case 6:count=4;break;default:Fail("native_color_format");}
   formats.push_back(std::uint8_t(f));stride+=count*4;
  }if(stride!=48)Fail("native_color_stride");
  auto &image=*s.color_target.image;auto *old_format=image._vertexFormat;const auto old_count=image._numVertexAttributes;const auto old_predefined=image._predefinedShader;const auto old_stride=image._vertexFormatStride;
  const auto &c=RoomValue(s.target)["native_context"]["color_modifier"];KColor tint(Num(c[0]),Num(c[1]),Num(c[2]),1);
  SourceQuad source{};source._coordinateSpace=SourceQuad::NORMALIZED_UV;
  const float u=float(image.GetWidth())/image.GetPaddedWidth(),v=float(image.GetHeight())/image.GetPaddedHeight();
  source._topRight=Vector(u,0);source._bottomLeft=Vector(0,v);source._bottomRight=Vector(u,v);
  DestinationQuad dest{};dest._topRight=Vector(g_OrthographicWidth,0);dest._bottomLeft=Vector(0,g_OrthographicHeight);dest._bottomRight=Vector(g_OrthographicWidth,g_OrthographicHeight);
  manager._blendMode=BlendMode(0,1,0,1,0);manager._shader=shader;
  image._predefinedShader=3;image._vertexFormat=formats.data();image._numVertexAttributes=std::uint8_t(formats.size());image._vertexFormatStride=std::uint16_t(stride);
  // Image::Render创建本地方形后，按J273颜色shader的三个具名参数补值；不是源几何重放。
  auto *vertices=image.Render(source,dest,tint);
  image._predefinedShader=old_predefined;image._vertexFormat=old_format;image._numVertexAttributes=old_count;image._vertexFormatStride=old_stride;
  if(!vertices)Fail("native_color_vertices");
  // Image::Render会按当前原生图像路径换算坐标；后处理必须使用Game.Render同型的屏幕正交矩形，
  // 不能再次套Room的0.65世界坐标缩放。这里只写本地颜色pass的四角和六个颜色参数。
  for(unsigned i=0;i<4;++i){float *p=vertices+12*i;p[0]=i%2?g_OrthographicWidth:0;p[1]=i/2?g_OrthographicHeight:0;p[2]=-1;
   p[3]=Num(c[0]);p[4]=Num(c[1]);p[5]=Num(c[2]);p[6]=1;p[7]=i%2?u:0;p[8]=i/2?v:0;
   p[9]=Num(c[3]);p[10]=Num(c[4]);p[11]=Num(c[5]);}
 }catch(...){restore();throw;}restore();
}
