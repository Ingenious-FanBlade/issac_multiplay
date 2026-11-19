import sys,time,json
sys.path.insert(0,'.scratch')
from step15_sample import query
cursor={27091:0,27092:0}
for p in cursor:query(p,'cs stream_test step15-perf-20261116 on')
start=time.monotonic()
with open(sys.argv[1],'w',encoding='utf-8') as f:
 while time.monotonic()-start<20:
  for p in cursor:
   for i in range(200):
    d=json.loads(query(p,'cs stream_test step15-perf-20261116 trace '+str(cursor[p]))['data']);cursor[p]=d['next']
    selected=[r for r in d['rows'] if r.get('event') in ('stream-progress','packet')]
    if selected:f.write(json.dumps({'port':p,'rows':selected,'lost':d.get('lost')})+'\n');f.flush()
    if not d['more']:break
  time.sleep(.03)
for p in cursor:query(p,'cs stream_test step15-perf-20261116 off')
print('complete')
