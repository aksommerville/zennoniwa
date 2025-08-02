all:
.SILENT:

EGG_SDK:=../egg2

all:;$(EGG_SDK)/out/eggdev build
clean:;rm -rf mid out
run:;$(EGG_SDK)/out/eggdev run
web-run:all;$(EGG_SDK)/out/eggdev serve --htdocs=out --project=.
edit:;$(EGG_SDK)/out/eggdev serve --project=. \
  --htdocs=/data:src/data \
  --htdocs=EGG_SDK/src/editor \
  --htdocs=src/editor \
  --htdocs=/out:out \
  --htdocs=EGG_SDK/src/web \
  --writeable=src/data

HARDCOPY:=etc/zennoniwa.html
all:$(HARDCOPY)
$(HARDCOPY):out/zennoniwa-web.html;cp $< $@
