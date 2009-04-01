#!/bin/csh

cd ../src

foreach i (*.c)
  ../script/dorm2 $i
  mv $i.out $i
end
foreach a (NSYSUBBS MULTIBBS DEBUG CSBBS HAVE_BUG SYSOP_BIN ACTFILE)
  foreach i (*.c)
    ../script/dorm3 $i $a
    mv $i.out $i
  end
end
foreach a (HAVE_DELONEUSER)
  foreach i (*.c)
    ../script/dorm3 $i $a
    mv $i.out $i
  end
end

cd ../csbbs

foreach i (*.c)
  ../script/dorm2 $i
  mv $i.out $i
end
foreach a (CATCH_CRACKER HAVE_BUG MULTIBBS)
  foreach i (*.c)
    ../script/dorm3 $i $a
    mv $i.out $i
  end
end

cd ../util
foreach i (*.c)
  ../script/dorm2 $i
  mv $i.out $i
end
foreach a (DEBUG MULTIBBS NSYSUBBS)
  foreach i (*.c)
    ../script/dorm3 $i $a
    mv $i.out $i
  end
end

cd ..
chown -R bbs:bbs *
