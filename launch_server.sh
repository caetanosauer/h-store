#!/bin/sh

java -ea -server -Xmx2048m -Xcheck:jni -cp "./obj/release/benchmarks/:./obj/release/prod:./third_party/java/jars/*" -Djava.library.path=obj/release/nativelibs edu.brown.hstore.HStore catalog.jar=tpcc.jar site.id=0 conf=properties/default.properties $*
