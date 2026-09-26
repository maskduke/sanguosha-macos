"""Bundle verified local dependencies, audit load paths, sign and archive the preview."""
import argparse,hashlib,json,pathlib,shutil,subprocess
p=argparse.ArgumentParser();p.add_argument('--source',required=True);p.add_argument('--destination',required=True);a=p.parse_args()
root=pathlib.Path(__file__).resolve().parent.parent;source=pathlib.Path(a.source).resolve();app=pathlib.Path(a.destination).resolve()
if app.exists(): raise SystemExit('Refusing to overwrite an existing delivery; select a new preview directory.')
lock=json.loads((root/'dependencies.lock.json').read_text())
for dep in lock['dependencies']:
 installed=pathlib.Path('/opt/homebrew/opt')/dep['name']
 if installed.resolve().name!=dep['version'] or hashlib.sha256((installed/'INSTALL_RECEIPT.json').read_bytes()).hexdigest()!=dep['receipt_sha256']:
  raise SystemExit('Dependency changed since validation: '+dep['name'])
app.parent.mkdir(parents=True,exist_ok=True);shutil.copytree(source,app,symlinks=True)
def run(*args):return subprocess.check_output([str(x) for x in args],stderr=subprocess.STDOUT,text=True)
# Explicit Qt plugin set for this Widgets/PNG/JPEG/SQLite application. Avoid
# globally installed, unrelated QtSvg/QML plugins discovered by macdeployqt.
plugins=[]
plugin_root=pathlib.Path('/opt/homebrew/opt/qtbase/share/qt/plugins')
for group in ['imageformats','sqldrivers','tls','networkinformation','styles']:
 plugins.extend((plugin_root/group).glob('*.dylib'))
plugins.append(plugin_root/'platforms/libqcocoa.dylib')
executables=[]
for plugin in plugins:
 target=app/'Contents/PlugIns'/plugin.relative_to(plugin_root)
 target.parent.mkdir(parents=True,exist_ok=True);shutil.copy(plugin,target)
 executables.append('-executable='+str(target))
deployment=run('/opt/homebrew/opt/qtbase/bin/macdeployqt',app,'-no-strip','-no-codesign','-no-plugins','-verbose=1',*executables)
print(deployment,flush=True)
if 'ERROR:' in deployment:raise SystemExit('macdeployqt reported an unresolved dependency')
(app/'Contents/Resources/qt.conf').write_text('[Paths]\nPlugins = PlugIns\n')

resources=app/'Contents/Resources';licenses=resources/'licenses';licenses.mkdir(exist_ok=True)
shutil.copy(root/'dependencies.lock.json',resources/'dependencies.lock.json');shutil.copy(root/'LICENSE',licenses/'QSanguosha-and-Qt-GPL-3.0.txt');shutil.copy(root/'third_party/miniaudio/LICENSE',licenses/'miniaudio.txt')
(licenses/'Lua-5.1.4.txt').write_text((root/'src/lua/lua.h').read_text().split('/******************************************************************************')[1] if '/******************************************************************************' in (root/'src/lua/lua.h').read_text() else (root/'src/lua/lua.h').read_text())
for package in ['libvorbis','libogg']:
 shutil.copy(pathlib.Path('/opt/homebrew/opt')/package/'COPYING',licenses/(package+'.txt'))
for dep in lock['dependencies']:
 directory=pathlib.Path('/opt/homebrew/opt')/dep['name']
 dest=licenses/dep['name'];dest.mkdir(exist_ok=True)
 for pattern in ['LICENSE*','COPYING*','NOTICE*','AUTHORS*']:
  for file in directory.glob(pattern):
   if file.is_file():shutil.copy(file,dest/file.name)
 sbom=directory/'sbom.spdx.json'
 if sbom.exists():shutil.copy(sbom,dest/'sbom.spdx.json')
(licenses/'NOTICE.txt').write_text('Local manual-acceptance preview. QSanguosha and bundled Qt are used under GPL-3.0; see the enclosed license. Lua is MIT; Vorbis/Ogg are BSD; miniaudio is MIT or Unlicense. Dependency versions, upstream source URLs and package SBOM notices are enclosed. Original game art/audio are preserved for this user-authorized local port. No public distribution has been performed.\n')
# macdeployqt deploys transitive non-Qt dylibs too. Reject any residual build paths.
machos=[];inventory=[]
for path in app.rglob('*'):
 if not path.is_file() or path.is_symlink():continue
 if path.open('rb').read(4) not in (b'\xcf\xfa\xed\xfe',b'\xca\xfe\xba\xbe'):continue
 info=run('/usr/bin/lipo','-archs',path).strip()
 if info!='arm64':raise SystemExit(f'Non-arm64 image: {path}: {info}')
 identity=run('/usr/bin/otool','-D',path).splitlines()
 if len(identity)>1 and identity[1].startswith('/opt/homebrew/'):
  run('/usr/bin/install_name_tool','-id','@rpath/'+str(path.relative_to(app/'Contents/Frameworks')),path)
 deps=run('/usr/bin/otool','-L',path)
 if '/opt/homebrew/' in '\n'.join(deps.splitlines()[1:]) or '/Users/' in '\n'.join(deps.splitlines()[1:]):raise SystemExit(f'External dependency: {deps}')
 load=run('/usr/bin/otool','-l',path)
 lines=load.splitlines()
 for i,line in enumerate(lines):
  if 'cmd LC_RPATH' in line:
   rpath=lines[i+2].strip().split(' ')[1]
   if rpath.startswith(('/opt/homebrew/','/Users/')):run('/usr/bin/install_name_tool','-delete_rpath',rpath,path)
 machos.append(path);inventory.append({'file':str(path.relative_to(app)),'arch':info,'dependencies':deps.splitlines()[1:]})
(resources/'dependency-audit.json').write_text(json.dumps(inventory,indent=2)+'\n')
for binary in machos:print(run('/usr/bin/codesign','--force','--sign','-',binary),end='')
for framework in (app/'Contents/Frameworks').glob('*.framework'):print(run('/usr/bin/codesign','--force','--sign','-',framework),end='')
print(run('/usr/bin/codesign','--force','--sign','-',app))
print(run('/usr/bin/codesign','--verify','--deep','--strict','--verbose=2',app))
zip_path=app.parent/(app.stem+'-arm64.zip');run('/usr/bin/ditto','-c','-k','--sequesterRsrc','--keepParent',app,zip_path)
sha=hashlib.sha256(zip_path.read_bytes()).hexdigest();zip_path.with_suffix('.zip.sha256').write_text(sha+'  '+zip_path.name+'\n')
print(json.dumps({'app':str(app),'zip':str(zip_path),'sha256':sha,'mach_o_images':len(machos)},indent=2))
