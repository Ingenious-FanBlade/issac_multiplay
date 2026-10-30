-- 整局规则与具名时钟保留各自原值；概率修正不冒充最终显示概率。
local X=include("domains/common/extended")
local R=X.Read
local schemas=include("domains/run/schema");R.AddSchemas(schemas)
local M={SchemaVersion=2}
function M.Capture(limits)
  R.Limits(limits);local c=X.Context();local g=Game();c.objects=1
  local out={location=X.Shared.Context(),state=R.Collect(g,"Game",c,0),ambush=R.Collect(Ambush,"Ambush",c,0),seeds=R.Collect(g:GetSeeds(),"Seeds",c,0),clocks={game_frame={value=g:GetFrameCount(),unit="simulation_frame_API",phase="OrdinaryPost"},time_counter={value=g.TimeCounter,unit="game_timer_frame_API"},application_frame={value=Isaac.GetFrameCount(),unit="application_frame_API"},authority={source="envelope.context.authority_tick",unit="AuthorityTick"}},probabilities={angel_modifier={value=g:GetLevel():GetAngelRoomChance(),source="Level.GetAngelRoomChance",semantics="modifier_not_final_probability"},devil_final={value=g:GetRoom():GetDevilRoomChance(),source="Room.GetDevilRoomChance"},planetarium_final={value=g:GetLevel():GetPlanetariumChance(),source="Level.GetPlanetariumChance"},conditions={schema="isaac.cs.level",semantics="canonical_raw_level_conditions;not_recomputed"}},sampling={freshness="ReadInCurrentNativeWindow",clock_semantics="pause_loading_and_render_clocks_remain_distinct"}}
  return R.Finish(out,c)
end
function M.Outcome() end
function M.Event(name,...)
  local c=X.Context();X.Publish(5,{event=name,arguments=R.Value(table.pack(...),nil,c,0),game_frame=Game():GetFrameCount(),application_frame=Isaac.GetFrameCount(),time_counter=Game().TimeCounter},0,c)
end
function M.Boundary(name) X.Shared.Boundary(5,name) end
function M.Query(object,class,member,args) return X.Query(5,schemas,object,class,member,args) end
return M
