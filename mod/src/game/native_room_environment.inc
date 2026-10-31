// J273原生FX的具名纯值适配。容器根由本DLL持有；引擎只借用，退休先解除再析构Room。
// 不截获图像像素、顶点或draw命令，图片始终由Manager的原版资源加载器解析。
struct FxInstance {float x=0,y=0,vx=0,vy=0;bool flip_x=false,flip_y=false;unsigned char padding[2]{};};
struct FxEntry {int id=0;KAGE_SmartPointer_ImageBase image;std::vector<FxInstance> instances;int count=0;
 float velocity_x_min=0,velocity_x_max=0,velocity_y_min=0,velocity_y_max=0;
 // 96CC00/96DB20按DWORD分派：这里是三个整数判别值，不能以次正规float编码再恢复。
 int render_pass=0,configured_stage=0,configuration_kind=0;
 unsigned char flag=0,position_mode=0,padding[2]{};float depth_scale=0;int mode=0,blend=1;bool projected=false;unsigned char tail[3]{};};
struct FxRay {float x=0,y=0,opacity=0;KAGE_SmartPointer_ImageBase image;};
struct FxRayGroup {float x_min=0,x_max=0,y_min=0,y_max=0,ray_spacing=0,group_spacing=0,phase=0;
 std::vector<FxRay> rays;bool enabled=false,alternate=false;unsigned char padding[2]{};};
struct FxVein {int sprite=0;FxVein *children[3]{};int frame=0;float rotation=0,scale=1,opacity=0;int mode=0,depth=0;float x=0,y=0;bool active=false;unsigned char padding[3]{};};
static_assert(sizeof(FxEntry)==76&&sizeof(FxInstance)==20&&sizeof(FxRay)==20&&sizeof(FxRayGroup)==44&&sizeof(FxVein)==52,"J273 FX layout");
struct Environment final {
 std::vector<FxEntry> layers;std::vector<FxRayGroup> rays;std::vector<std::unique_ptr<FxVein>> branches;
 // 根只持有本方树的借用指针，析构时由branches统一释放，不调用原生递归delete。
 FxVein roots[10]{};
 FxEntry poop;
 std::vector<std::unique_ptr<Environment>> retired;
};
std::unique_ptr<Environment> EnvironmentCreate(){return std::make_unique<Environment>();}
template<class T>T &FxAt(void *p,unsigned offset){return *reinterpret_cast<T *>(static_cast<unsigned char *>(p)+offset);}
template<class T>const T &FxAt(const void *p,unsigned offset){return *reinterpret_cast<const T *>(static_cast<const unsigned char *>(p)+offset);}
template<class T>void FxBorrow(void *p,unsigned offset,std::vector<T> &values){
 T *range[]={values.data(),values.data()+values.size(),values.data()+values.size()};std::memcpy(static_cast<char *>(p)+offset,range,sizeof(range));
}
void EnvironmentDetach(Environment &,Room *room){auto *fx=&room->_fxLayers;std::memset(reinterpret_cast<char *>(fx)+0x18,0,24);
 FxAt<KAGE_SmartPointer_ImageBase>(fx,0x3c)=KAGE_SmartPointer_ImageBase();std::memset(reinterpret_cast<char *>(fx)+0x44,0,12);
 for(unsigned i=0;i<10;++i)for(unsigned c=0;c<3;++c)FxAt<FxVein>(fx,0xc0+i*52).children[c]=nullptr;
}
std::string FxImageName(const KAGE_SmartPointer_ImageBase &p){if(!p.image)return {};if(!p.image->_name)Fail("native_fx_image_name");std::string path=p.image->_name;
 if(path.rfind("resources/",0)==0)path.erase(0,10);return Path(V(path.data(),unsigned(path.size())));}
void FxImage(KAGE_SmartPointer_ImageBase &p,const V &v){const auto name=Str(v);if(name.empty()){p=KAGE_SmartPointer_ImageBase();return;}
 const auto path=Path(v);KAGE_SmartPointer_ImageBase image;
 ResourceFile(v);
 Manager::LoadImage(&image,path.c_str(),__ptr_g_VertexAttributeDescriptor_Position,false);
 if(!image.image)Fail("native_fx_resource");p=std::move(image);
}
// 字段表的偏移只在SDK适配内部使用；报文包含有类型的具名数值，不携带内存块／指针。
const std::pair<const char *,unsigned> fx_floats[]={{"velocity_x_min",28},{"velocity_x_max",32},{"velocity_y_min",36},{"velocity_y_max",40},{"depth_scale",60}};
const std::pair<const char *,unsigned> fx_ints[]={{"render_pass",44},{"configured_stage",48},{"configuration_kind",52}};
const std::pair<const char *,unsigned> ray_floats[]={{"x_min",0},{"x_max",4},{"y_min",8},{"y_max",12},{"ray_spacing",16},{"group_spacing",20},{"phase",24}};
const std::pair<const char *,unsigned> vein_floats[]={{"rotation",20},{"scale",24},{"opacity",28},{"x",40},{"y",44}};
template<std::size_t N>void FxCaptureFloats(V &out,const void *p,const std::pair<const char *,unsigned>(&fields)[N],A &a){for(const auto &f:fields){const auto n=FxAt<float>(p,f.second);if(!std::isfinite(n))Fail("native_fx_nonfinite");Put(out,f.first,V(n),a);}}
template<std::size_t N>void FxApplyFloats(void *p,const V &v,const std::pair<const char *,unsigned>(&fields)[N]){for(const auto &f:fields)FxAt<float>(p,f.second)=Num(At(v,f.first));}
V FxCaptureVein(const FxVein &v,A &a,unsigned depth,unsigned &count){if(depth>6||++count>4096)Fail("native_fx_vein_budget");V out(rapidjson::kObjectType);
 FxCaptureFloats(out,&v,vein_floats,a);Put(out,"sprite",V(v.sprite),a);Put(out,"frame",V(v.frame),a);Put(out,"mode",V(v.mode),a);Put(out,"depth",V(v.depth),a);Put(out,"active",V(v.active),a);
 V children(rapidjson::kArrayType);for(auto *child:v.children)children.PushBack(child?FxCaptureVein(*child,a,depth+1,count):V(),a);Put(out,"children",std::move(children),a);return out;
}
void FxApplyVein(Environment &e,FxVein &v,const V &state,unsigned depth){if(depth>6||e.branches.size()>4096)Fail("native_fx_vein_budget");FxApplyFloats(&v,state,vein_floats);
 v.sprite=int(Int(state["sprite"]));v.frame=int(Int(state["frame"]));v.mode=int(Int(state["mode"]));v.depth=int(Int(state["depth"]));v.active=Flag(state["active"]);
 const auto &children=At(state,"children");if(!children.IsArray()||children.Size()!=3||v.sprite<0||v.sprite>1)Fail("native_fx_vein");
 for(unsigned i=0;i<3;++i)if(!children[i].IsNull()){auto node=std::make_unique<FxVein>();auto *child=node.get();e.branches.push_back(std::move(node));v.children[i]=child;FxApplyVein(e,*child,children[i],depth+1);}else v.children[i]=nullptr;
}
template<class T>const std::vector<T> &FxRange(const void *fx,unsigned offset){const auto &v=FxAt<std::vector<T>>(fx,offset);if(v.size()>4096)Fail("native_fx_collection");return v;}
V FxCaptureEntry(const FxEntry &v,A &a,bool poop=false){V item(rapidjson::kObjectType);
 if(!poop)Put(item,"id",V(v.id),a);Put(item,"image",S(FxImageName(v.image),a),a);
 for(const auto &f:fx_ints)if(!poop||f.second!=44)Put(item,f.first,V(FxAt<int>(&v,f.second)),a);
 Put(item,"count",V(v.count),a);Put(item,"flag",V(unsigned(v.flag)),a);Put(item,"position_mode",V(unsigned(v.position_mode)),a);Put(item,"mode",V(v.mode),a);Put(item,"blend",V(v.blend),a);Put(item,"projected",V(v.projected),a);
 // 内嵌便便云没有XML id／opacity／depth初始化，这三项并非该分支的消费字段。
 for(const auto &f:fx_floats)if(!poop||f.second!=60){const auto number=FxAt<float>(&v,f.second);if(!std::isfinite(number))Fail("native_fx_nonfinite");Put(item,f.first,V(number),a);}
 V instances(rapidjson::kArrayType);for(const auto &p:v.instances){if(instances.Size()>=4096)Fail("native_fx_instances");V point(rapidjson::kObjectType);
  Put(point,"x",V(p.x),a);Put(point,"y",V(p.y),a);Put(point,"vx",V(p.vx),a);Put(point,"vy",V(p.vy),a);Put(point,"flip_x",V(p.flip_x),a);Put(point,"flip_y",V(p.flip_y),a);instances.PushBack(std::move(point),a);}
 Put(item,"instances",std::move(instances),a);return item;
}
FxEntry FxReadEntry(const V &v,bool poop=false){FxEntry item;if(!poop)item.id=int(Int(v["id"]));item.count=int(Int(v["count"]));item.flag=static_cast<unsigned char>(Int(v["flag"]));item.position_mode=static_cast<unsigned char>(Int(v["position_mode"]));
 item.mode=int(Int(v["mode"]));item.blend=int(Int(v["blend"]));item.projected=Flag(v["projected"]);for(const auto &f:fx_floats)if(!poop||f.second!=60)FxAt<float>(&item,f.second)=Num(At(v,f.first));
 for(const auto &f:fx_ints)if(!poop||f.second!=44)FxAt<int>(&item,f.second)=int(Int(At(v,f.first)));FxImage(item.image,v["image"]);
 for(const auto &p:v["instances"].GetArray()){FxInstance point;point.x=Num(p["x"]);point.y=Num(p["y"]);point.vx=Num(p["vx"]);point.vy=Num(p["vy"]);point.flip_x=Flag(p["flip_x"]);point.flip_y=Flag(p["flip_y"]);item.instances.push_back(point);}return item;
}
V FxVector(const Vector &p,A &a){V v(rapidjson::kObjectType);Put(v,"type",S("vector",a),a);Put(v,"x",V(p.x),a);Put(v,"y",V(p.y),a);return v;}
V EnvironmentCapture(Room *room,A &a){auto *fx=&room->_fxLayers;V out(rapidjson::kObjectType);Put(out,"loaded",V(fx->_loaded),a);
 Put(out,"selected_ray",V(FxAt<int>(fx,0x30)),a);Put(out,"poop_countdown",V(fx->_poopFxCountdown),a);
 V layers(rapidjson::kArrayType);for(const auto &v:FxRange<FxEntry>(fx,0x18))layers.PushBack(FxCaptureEntry(v,a),a);Put(out,"layers",std::move(layers),a);
 if(fx->_poopFxCountdown>0)Put(out,"poop",FxCaptureEntry(FxAt<FxEntry>(fx,0x38),a,true),a);else Put(out,"poop",V(),a);
 V color(rapidjson::kArrayType);for(float c:fx->_poopFxColor._tint)color.PushBack(V(c),a);for(float c:fx->_poopFxColor._colorize)color.PushBack(V(c),a);for(float c:fx->_poopFxColor._offset)color.PushBack(V(c),a);Put(out,"poop_color",std::move(color),a);
 for(const auto &p:std::vector<std::pair<const char *,unsigned>>{{"player_anchor",0},{"vein_anchor",0x4f0},{"bounds_start",0xb0},{"bounds_end",0xb8}})Put(out,p.first,FxVector(FxAt<Vector>(fx,p.second),a),a);
 V groups(rapidjson::kArrayType);for(const auto &v:FxRange<FxRayGroup>(fx,0x24)){V group(rapidjson::kObjectType);FxCaptureFloats(group,&v,ray_floats,a);Put(group,"enabled",V(v.enabled),a);Put(group,"alternate",V(v.alternate),a);
  V rays(rapidjson::kArrayType);for(const auto &p:v.rays){if(rays.Size()>=4096)Fail("native_fx_rays");V ray(rapidjson::kObjectType);Put(ray,"x",V(p.x),a);Put(ray,"y",V(p.y),a);Put(ray,"opacity",V(p.opacity),a);Put(ray,"image",S(FxImageName(p.image),a),a);rays.PushBack(std::move(ray),a);}Put(group,"rays",std::move(rays),a);groups.PushBack(std::move(group),a);
 }Put(out,"rays",std::move(groups),a);
 V veins(rapidjson::kArrayType);unsigned count=0;for(unsigned i=0;i<10;++i)veins.PushBack(FxCaptureVein(FxAt<FxVein>(fx,0xc0+52*i),a,0,count),a);Put(out,"veins",std::move(veins),a);
 V sprites(rapidjson::kArrayType);for(unsigned i=0;i<2;++i){auto pose=Parse(Lua("CaptureSprite",&FxAt<ANM2>(fx,0x2c8+sizeof(ANM2)*i),lua::Metatables::SPRITE,nullptr,true));sprites.PushBack(V(pose,a),a);}Put(out,"sprites",std::move(sprites),a);
 return out;
}
void EnvironmentApply(Environment &e,Room *room,const V &state){Environment next;auto *fx=&room->_fxLayers;
 for(const auto &v:At(state,"layers").GetArray())next.layers.push_back(FxReadEntry(v));if(!At(state,"poop").IsNull())next.poop=FxReadEntry(state["poop"],true);
 for(const auto &v:At(state,"rays").GetArray()){FxRayGroup group;FxApplyFloats(&group,v,ray_floats);group.enabled=Flag(v["enabled"]);group.alternate=Flag(v["alternate"]);
  for(const auto &p:v["rays"].GetArray()){FxRay ray;ray.x=Num(p["x"]);ray.y=Num(p["y"]);ray.opacity=Num(p["opacity"]);FxImage(ray.image,p["image"]);group.rays.push_back(std::move(ray));}next.rays.push_back(std::move(group));
 }
 const auto &veins=At(state,"veins"),&sprites=At(state,"sprites");if(veins.Size()!=10||sprites.Size()!=2)Fail("native_fx_veins");for(unsigned i=0;i<10;++i)FxApplyVein(next,next.roots[i],veins[i],0);
 // 先准备完图片和树，再切换借用；旧容器直到本帧GPU队列排空后才能退休，由Scene生命周期保护。
 EnvironmentDetach(e,room);auto previous=std::make_unique<Environment>(std::move(e));e=std::move(next);e.retired.push_back(std::move(previous));FxBorrow(fx,0x18,e.layers);FxBorrow(fx,0x24,e.rays);
 // 内嵌便便云与普通FX是同一76字节结构，图片独立加引用，实例数组仍归本方owner。
 auto &poop=FxAt<FxEntry>(fx,0x38);poop.id=e.poop.id;poop.image=e.poop.image;FxBorrow(fx,0x44,e.poop.instances);poop.count=e.poop.count;
 for(const auto &f:fx_floats)FxAt<float>(&poop,f.second)=FxAt<float>(&e.poop,f.second);poop.flag=e.poop.flag;poop.position_mode=e.poop.position_mode;poop.mode=e.poop.mode;poop.blend=e.poop.blend;poop.projected=e.poop.projected;
 for(const auto &f:fx_ints)FxAt<int>(&poop,f.second)=FxAt<int>(&e.poop,f.second);
 unsigned ci=0;const auto &color=state["poop_color"];for(float &c:fx->_poopFxColor._tint)c=Num(color[ci++]);for(float &c:fx->_poopFxColor._colorize)c=Num(color[ci++]);for(float &c:fx->_poopFxColor._offset)c=Num(color[ci++]);
 for(const auto &p:std::vector<std::pair<const char *,unsigned>>{{"player_anchor",0},{"vein_anchor",0x4f0},{"bounds_start",0xb0},{"bounds_end",0xb8}})FxAt<Vector>(fx,p.second)=Vec(state[p.first]);
 for(unsigned i=0;i<10;++i)FxAt<FxVein>(fx,0xc0+52*i)=e.roots[i];
 for(unsigned i=0;i<2;++i)Sprite(FxAt<ANM2>(fx,0x2c8+sizeof(ANM2)*i),sprites[i]);
 fx->_loaded=Flag(state["loaded"]);FxAt<int>(fx,0x30)=int(Int(state["selected_ray"]));fx->_poopFxCountdown=int(Int(state["poop_countdown"]));
 FxAt<unsigned>(fx,0xc)=g_Game->_stage;FxAt<unsigned>(fx,0x10)=g_Game->_stageType;FxAt<unsigned>(fx,0x14)=room->_backdrop.backdropId;
}
