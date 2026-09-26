"""Fail-fast native integration checks; never repeat a crashing test automatically."""
import argparse,json,os,pathlib,subprocess,sys,time
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);p.add_argument('--games',type=int,default=20);a=p.parse_args()
out=pathlib.Path(a.output);out.mkdir(parents=True,exist_ok=True);results=[]
cases=[('--smoke-test','02p')]+[('--shutdown-smoke',m) for m in ['02p','06_3v3','02_1v1','04_1v3']]+[('--ai-smoke',['02p','04p','08p','06_3v3','02_1v1'][i%5]) for i in range(a.games)]
for i,(flag,mode) in enumerate(cases,1):
 env=dict(os.environ,QS_DATA_DIR='/private/tmp/qs-game-checks',QS_TEST_MODE=mode)
 with (out/f'{i:02}-{mode}-{flag[2:]}.log').open('wb') as log:
  started=time.monotonic()
  child=subprocess.Popen([a.app,flag],env=env,stdout=log,stderr=subprocess.STDOUT)
  try: code=child.wait(timeout=185)
  except subprocess.TimeoutExpired:
   child.terminate()
   try: child.wait(timeout=3)
   except subprocess.TimeoutExpired: child.kill();child.wait()
   code=124
 text=(out/f'{i:02}-{mode}-{flag[2:]}.log').read_text(errors='replace')
 expected={'--smoke-test':'NIRVANA_SMOKE','--shutdown-smoke':'NIRVANA_SHUTDOWN_TEST','--ai-smoke':'NIRVANA_AI_COMPLETE'}[flag]
 result={'test':flag,'mode':mode,'exit':code,'duration_seconds':round(time.monotonic()-started,2),'pass':code==0 and expected in text and 'stack traceback' not in text};results.append(result)
 (out/'summary.json').write_text(json.dumps(results,indent=2)+'\n');print(result,flush=True)
 if not result['pass']: sys.exit(1)
