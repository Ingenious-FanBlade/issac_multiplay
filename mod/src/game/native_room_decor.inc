// 原生墙缓存的语义选择见证。只保留当前Authority Backdrop的有界纯值，关闭Recorder也有效。
// Entity helper内部会推进Backdrop私有RNG；Client屏蔽Entity时仍恢复该原生后态，后续墙格不重抽。
struct DecorChoiceValue {int x=0,y=0,kind=0;unsigned layer=0,seed=0;float px=0,py=0;bool fx=false,fy=false;};
struct DecorSource {Backdrop *owner=nullptr;RNG entry{};unsigned backdrop=0,seed=0;bool active=false,complete=false;std::vector<DecorChoiceValue> choices;};
DecorSource decor_source;
bool DecorOwner(Backdrop *b){return IsMain()&&g_Game&&g_Game->_room&&b==&g_Game->_room->_backdrop;}
void DecorBegin(Backdrop *b){
 if(bound){if(b!=&bound->room->_backdrop)Fail("native_decor_owner");const auto &v=RoomValue(bound->target)["native_context"]["wall_decor"];
  const auto &r=v["entry_rng"];b->rng._seed=unsigned(Int(r["seed"]));b->rng._shift1=unsigned(Int(r["shift1"]));b->rng._shift2=unsigned(Int(r["shift2"]));b->rng._shift3=unsigned(Int(r["shift3"]));bound->decor_cursor=0;return;}
 if(!DecorOwner(b))return;decor_source.owner=b;decor_source.entry=b->rng;decor_source.backdrop=b->backdropId;decor_source.seed=b->seed;
 decor_source.choices.clear();decor_source.active=true;decor_source.complete=false;
}
void DecorEnd(Backdrop *b,bool success){
 if(bound){if(success&&bound->decor_cursor!=RoomValue(bound->target)["native_context"]["wall_decor"]["choices"].Size())Fail("native_decor_choice_count");return;}
 if(decor_source.owner==b&&decor_source.active){decor_source.active=false;decor_source.complete=success;}
}
void DecorChoice(Backdrop *b,int x,int y,unsigned layer,int kind,float px,float py,bool fx,bool fy,bool returned){
 if(bound){if(returned)return;const auto &rows=RoomValue(bound->target)["native_context"]["wall_decor"]["choices"];
  if(bound->decor_cursor>=rows.Size())Fail("native_decor_choice_count");const auto &v=rows[bound->decor_cursor++];
  if(Int(v["x"])!=x||Int(v["y"])!=y||Int(v["layer"])!=layer||Int(v["kind"])!=kind||Flag(v["flip_x"])!=fx||Flag(v["flip_y"])!=fy||
     std::abs(Num(v["px"])-px)>0.0001f||std::abs(Num(v["py"])-py)>0.0001f)Fail("native_decor_choice_mismatch");
  b->rng._seed=unsigned(Int(v["after_seed"]));return;}
 if(!returned||!decor_source.active||decor_source.owner!=b)return;
 // 来源见证失败不改变Host原调用；该缓存保持不完整，后续正式采集显式拒绝。
 try{if(decor_source.choices.size()>=4096){decor_source.complete=false;decor_source.active=false;return;}
  decor_source.choices.push_back({x,y,kind,layer,b->rng._seed,px,py,fx,fy});}
 catch(...){decor_source.complete=false;decor_source.active=false;}
}
V DecorCapture(Backdrop *b,A &a){
 if(decor_source.owner!=b||!decor_source.complete||decor_source.active||decor_source.backdrop!=b->backdropId||decor_source.seed!=b->seed)Fail("native_source_wall_decor_incomplete");
 V out(rapidjson::kObjectType),rng(rapidjson::kObjectType);const auto &r=decor_source.entry;
 Put(rng,"seed",V(r._seed),a);Put(rng,"shift1",V(r._shift1),a);Put(rng,"shift2",V(r._shift2),a);Put(rng,"shift3",V(r._shift3),a);Put(out,"entry_rng",std::move(rng),a);
 V rows(rapidjson::kArrayType);for(const auto &c:decor_source.choices){V v(rapidjson::kObjectType);
  Put(v,"x",V(c.x),a);Put(v,"y",V(c.y),a);Put(v,"layer",V(c.layer),a);Put(v,"kind",V(c.kind),a);Put(v,"px",V(c.px),a);Put(v,"py",V(c.py),a);
  Put(v,"flip_x",V(c.fx),a);Put(v,"flip_y",V(c.fy),a);Put(v,"after_seed",V(c.seed),a);rows.PushBack(std::move(v),a);}
 Put(out,"choices",std::move(rows),a);return out;
}
