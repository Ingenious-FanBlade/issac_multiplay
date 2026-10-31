// 公开GetFrame截断小数相位；此补充保留原生已选择的轨道key，不在Client推进动画或重触发事件。
std::map<std::string,ANM2 *> MotionSprites(Room *room){std::map<std::string,ANM2 *> out;
 auto **slots=reinterpret_cast<GridEntity **>(reinterpret_cast<char *>(room)+0x24);
 for(unsigned i=0;i<448;++i)if(auto *g=slots[i]){out.emplace("grid-"+std::to_string(i),&g->_sprite);
  if(g->GetType()==16)out.emplace("extra-"+std::to_string(i),&static_cast<GridEntity_Door *>(g)->_extraSprite);
  if(g->GetType()==20)out.emplace("timer-"+std::to_string(i),reinterpret_cast<ANM2 *>(reinterpret_cast<char *>(g)+0x154));
 }
 out.emplace("floor",&room->_backdrop.floorANM2);out.emplace("detail",&room->_backdrop._wallDetailANM2);
 out.emplace("controls",&room->_backdrop._controlsANM2);out.emplace("buttons",&room->_backdrop._controlsButtonsANM2);
 for(unsigned i=0;i<2;++i)out.emplace("vein-"+std::to_string(i),&FxAt<ANM2>(&room->_fxLayers,0x2c8+sizeof(ANM2)*i));return out;
}
V MotionState(const AnimationState &state,A &a){if(!state._animData)return V();const auto *data=state._animData;
 if(data->_layerCount<0||data->_layerCount>64||data->_nullLayerCount<0||data->_nullLayerCount>64||
   (data->_layerCount&&!state._layerFrames)||(data->_nullLayerCount&&!state._nullLayerFrames))Fail("native_motion_tracks");
 V out(rapidjson::kObjectType);Put(out,"animation",S(data->_name,a),a);Put(out,"frame",V(state._animFrame),a);V layers(rapidjson::kArrayType),nulls(rapidjson::kArrayType);
 for(int i=0;i<data->_layerCount;++i){V layer(rapidjson::kObjectType);Put(layer,"id",V(data->_animLayers[i]._layerID),a);Put(layer,"key",V(state._layerFrames[i]),a);layers.PushBack(std::move(layer),a);}
 for(int i=0;i<data->_nullLayerCount;++i)nulls.PushBack(V(state._nullLayerFrames[i]),a);Put(out,"layers",std::move(layers),a);Put(out,"nulls",std::move(nulls),a);return out;
}
V MotionCapture(Room *room,A &a){V out(rapidjson::kObjectType);for(const auto &pair:MotionSprites(room)){V pose(rapidjson::kObjectType);auto *s=pair.second;
 Put(pose,"main",MotionState(s->_animState,a),a);Put(pose,"overlay",MotionState(s->_overlayAnimState,a),a);Put(pose,"overlay_first",V(s->_overlayRenderFirst),a);
 Put(out,pair.first.c_str(),std::move(pose),a);}return out;
}
void MotionApplyState(AnimationState &state,const V &value){if(value.IsNull()){if(state._animData)Fail("native_motion_absent_animation");return;}
 auto *data=state._animData;if(!data||data->_name!=Str(value["animation"])||data->_layerCount!=int(value["layers"].Size())||data->_nullLayerCount!=int(value["nulls"].Size()))Fail("native_motion_resource_tracks");
 for(int i=0;i<data->_layerCount;++i){const auto &v=value["layers"][i];const auto key=int(Int(v["key"]));const auto &track=data->_animLayers[i];
  // 无帧轨道的原生游标可保持0；绘制以轨道帧数判空，不把这个哨兵当可访问帧。
  const bool empty_cursor=track._numFrames==0&&key==0;
  if(track._layerID!=Int(v["id"])||key<-1||(!empty_cursor&&key>=track._numFrames))
   throw std::runtime_error("native_motion_layer_key:"+data->_name+":"+std::to_string(track._layerID)+":"+std::to_string(key)+"/"+std::to_string(track._numFrames));state._layerFrames[i]=key;}
 for(int i=0;i<data->_nullLayerCount;++i){const auto key=int(Int(value["nulls"][i]));if(key<-1||key>65535)Fail("native_motion_null_key");state._nullLayerFrames[i]=key;}
 state._animFrame=Num(value["frame"]);state._isPlaying=false;state._currentlyTriggeredEvents=state._previouslyTriggeredEvents=0;
}
void MotionApply(Room *room,const V &poses){const auto sprites=MotionSprites(room);if(sprites.size()!=poses.MemberCount())Fail("native_motion_membership");
 for(const auto &pair:sprites){const auto &pose=At(poses,pair.first.c_str());MotionApplyState(pair.second->_animState,pose["main"]);MotionApplyState(pair.second->_overlayAnimState,pose["overlay"]);pair.second->_overlayRenderFirst=Flag(pose["overlay_first"]);}
}
