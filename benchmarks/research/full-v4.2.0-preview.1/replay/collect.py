"""Sequential bounded experiment receipt collector; never overwrite a capture."""
from pathlib import Path
import argparse,csv,ctypes,gzip,hashlib,json,os,platform,subprocess,sys,time
FIELDS=['seconds','requested_live','requested_peak','alloc_calls','free_calls_before_destroy','requested_after_destroy','working_set','peak_working_set','records_reassigned','max_reassigned','splits','merges','redistributions','cross_block_moves','total_frees']
def available():
    if os.name=='nt':
        class M(ctypes.Structure):_fields_=[('length',ctypes.c_ulong),('load',ctypes.c_ulong)]+[(n,ctypes.c_ulonglong) for n in ('total','available','page_total','page_available','virtual_total','virtual_available','extended')]
        m=M();m.length=ctypes.sizeof(m);assert ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(m));return m.available
    return int(next(l.split()[1] for l in Path('/proc/meminfo').read_text().splitlines() if l.startswith('MemAvailable:')))*1024
def main():
    p=argparse.ArgumentParser();p.add_argument('--stable40',type=Path,required=True);p.add_argument('--stable41',type=Path,required=True);p.add_argument('--build',type=Path,required=True);p.add_argument('--traces',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--om-prod',type=Path);p.add_argument('--om-diag',type=Path);p.add_argument('--suite',choices=['components','database','locality'],required=True);a=p.parse_args()
    a.output.mkdir(parents=True,exist_ok=False);a.build=a.build.resolve();a.traces=a.traces.resolve();a.output=a.output.resolve();ext='.exe' if os.name=='nt' else ''
    meta=dict(platform=platform.platform(),processor=platform.processor(),python=sys.version,cpus=os.cpu_count(),research_sha=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),started_utc=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),suite=a.suite,guard_bytes=512*1024**2,timeout_seconds=120)
    if os.name!='nt':meta['cpuinfo']=Path('/proc/cpuinfo').read_text().split('\n\n')[0]
    meta['compiler']=subprocess.check_output(['gcc','--version'],text=True).splitlines()[0]
    binaries={};receipts=[];rows=[]
    def execute(command,label):
        command=list(map(str,command));binary=Path(command[0]);binaries[str(binary)]=hashlib.sha256(binary.read_bytes()).hexdigest();mem=available();receipt=dict(id=label,command=command,available=mem,utc=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()))
        if mem<512*1024**2:receipt.update(status='safely_skipped',reason='available memory below unchanged guard');receipts.append(receipt);return None
        try:
            r=subprocess.run(command,capture_output=True,timeout=120);receipt.update(status='ok' if r.returncode==0 else 'failed',returncode=r.returncode)
            (a.output/(label+'.stdout.gz')).write_bytes(gzip.compress(r.stdout,mtime=0));(a.output/(label+'.stderr.gz')).write_bytes(gzip.compress(r.stderr,mtime=0));receipts.append(receipt)
            assert r.returncode==0,(label,r.stderr.decode(errors='replace')[:1500]);return r.stdout.decode()
        except subprocess.TimeoutExpired as e:
            receipt.update(status='timeout');receipts.append(receipt)
            (a.output/(label+'.stdout.gz')).write_bytes(gzip.compress(e.stdout or b'',mtime=0));raise
        finally:
            (a.output/'invocations.jsonl').write_text(''.join(json.dumps(x)+'\n' for x in receipts))
    try:
        if a.suite=='locality':
            for mode in ('prod','diag'):
                text=execute([a.build/('locality_'+mode+ext)],'locality-'+mode)
                if text:(a.output/('locality-'+mode+'.csv')).write_text(text)
        else:
            if a.suite=='components':
                for kind in ('full','list','tree','tag','list_range'):
                    cmd=[(a.om_diag).resolve(),kind,a.traces/'semantic-gate.trace','1'] if kind in ('tag','list_range') else [a.build/(kind+'_diag'+ext),a.traces/'semantic-gate.trace','1']
                    execute(cmd,'semantic-gate-'+kind)
            manifest=json.loads((a.traces/'manifest.json').read_text())
            for t,info in enumerate(manifest):
                db=info['name'].startswith('db-')
                if db!=(a.suite=='database'):continue
                trace=a.traces/info['name'];assert hashlib.sha256(trace.read_bytes()).hexdigest()==info['sha256']
                if db:
                    variants=['memory','persistent'];modes=[('prod',r) for r in range(6)]
                else:
                    assert a.om_prod and a.om_diag
                    variants=['stable40','stable41','full','list','tree','tag','list_range'];modes=[('diag',0)]+[('prod',r) for r in range(8)]
                for mode,repeat in modes:
                    shift=(t+repeat)%len(variants);order=variants[shift:]+variants[:shift]
                    for kind in order:
                        label=f'{t}-{kind}-{mode}-{repeat}'
                        if db:
                            cmd=[a.build/('database_driver'+ext),trace,kind]
                            if kind=='persistent':cmd.append(a.output/('db-'+label))
                        elif kind in ('tag','list_range'):cmd=[(a.om_diag if mode=='diag' else a.om_prod).resolve(),kind,trace,'1' if mode=='diag' else '0']
                        elif kind in ('stable40','stable41'):cmd=[getattr(a,kind).resolve()/('full_'+mode+ext),trace,'1' if mode=='diag' else '0']
                        else:cmd=[a.build/(kind+'_'+mode+ext),trace,'1' if mode=='diag' else '0']
                        text=execute(cmd,label)
                        if text is None:continue
                        common=dict(n=info['n'],seed=info['seed'],scenario=info['scenario'],implementation=kind,mode=mode,repeat=repeat,trace_sha256=info['sha256'])
                        if db:
                            v=text.splitlines()[0].split(',');assert v[0]=='summary';assert len(text.splitlines())==1001
                            row=dict(common,setup_seconds=float(v[1]),seconds=float(v[2]),checkpoint_seconds=float(v[3]),reopen_seconds=float(v[4]),history_seconds=float(v[5]),receipts=1000)
                        else:
                            v=text.strip().split(',');assert len(v)==15
                            row=dict(common,**{k:float(x) if k=='seconds' else int(x) for k,x in zip(FIELDS,v)})
                            if kind in ('tag','list_range'):
                                for k in FIELDS[4:]:
                                    if k!='requested_after_destroy':row[k]=None
                            if mode=='prod':
                                for k in FIELDS[1:]:row[k]=None
                        rows.append(row)
                        if len(rows)%50==0:print(a.suite,len(rows),flush=True)
                keys=list(dict.fromkeys(k for r in rows for k in r))
                with (a.output/'raw.csv').open('w',newline='') as f:w=csv.DictWriter(f,keys);w.writeheader();w.writerows(rows)
    finally:
        meta.update(finished_utc=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),binaries=binaries,completed_rows=len(rows));(a.output/'metadata.json').write_text(json.dumps(meta,indent=2)+'\n')
if __name__=='__main__':main()
