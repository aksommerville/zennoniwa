all:
.SILENT:

all:;eggdev build
clean:;rm -rf mid out
run:;eggdev run
web-run:all;eggdev serve --htdocs=out --project=.
edit:;eggdev serve --htdocs=/data:src/data --htdocs=EGG_SDK/src/editor --htdocs=src/editor --htdocs=/out:out --writeable=src/data --project=.
