"""Prepare private runtime resources, leaving the original asset snapshot intact."""
import pathlib, shutil, sys, json
source, target = map(pathlib.Path, sys.argv[1:3])
target.mkdir(parents=True, exist_ok=True)
shutil.copytree(source, target, dirs_exist_ok=True, ignore=shutil.ignore_patterns('*.dll', '.DS_Store'))
script=target/'lua/sanguosha.lua'
text=script.read_text()
text=text.replace('package.path = package.path .. ";./lua/lib/?.lua"', 'package.path = "./?.lua;./lua/lib/?.lua"')
text=text.replace('package.cpath = package.cpath .. ";./lua/clib/?.dll"', 'package.cpath = "" -- Bundled game uses pure Lua and the built-in sgs module.')
script.write_text(text)
(target.parent/'build-info.json').write_text(json.dumps({'product':'QSanguosha Nirvana','preview':'0.1.0-preview.1','baseline':'323f112658b5ea0c081b02e240afb92af7a379c1','architecture':'arm64','min_macos':'26.0','status':'manual acceptance preview'},indent=2)+'\n')

patches=json.loads((pathlib.Path(__file__).resolve().parent.parent/'patches/runtime-lua.json').read_text())
for patch in patches:
    file=target/patch['file']
    text=file.read_text()
    if text.count(patch['old'])!=1: raise RuntimeError('Runtime patch does not match baseline: '+patch['id'])
    file.write_text(text.replace(patch['old'],patch['new']))
