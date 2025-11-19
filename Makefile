all:
.SILENT:

EGG_SDK:=../egg2

all:;$(EGG_SDK)/out/eggdev build
clean:;rm -rf mid out
run:;$(EGG_SDK)/out/eggdev run
web-run:all;$(EGG_SDK)/out/eggdev serve --htdocs=out/zennoniwa-web.zip --project=.
edit:;$(EGG_SDK)/out/eggdev serve --project=. \
  --htdocs=/data:src/data \
  --htdocs=EGG_SDK/src/editor \
  --htdocs=src/editor \
  --htdocs=/out:out \
  --htdocs=EGG_SDK/src/web \
  --htdocs=/synth.wasm:EGG_SDK/out/web/synth.wasm \
  --htdocs=/build:out/zennoniwa-web.zip \
  --writeable=src/data
