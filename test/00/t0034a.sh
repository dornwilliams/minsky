#! /bin/sh

here=`pwd`
if test $? -ne 0; then exit 2; fi
tmp=/tmp/$$
mkdir $tmp
if test $? -ne 0; then exit 2; fi
cd $tmp
if test $? -ne 0; then exit 2; fi

fail()
{
    echo "FAILED" 1>&2
    cd $here
    chmod -R u+w $tmp
    rm -rf $tmp
    exit 1
}

pass()
{
    echo "PASSED" 1>&2
    cd $here
    chmod -R u+w $tmp
    rm -rf $tmp
    exit 0
}

trap "fail" 1 2 3 15

# insert ecolab script code here
# use \$ in place of $ to refer to variable contents
# exit 0 to indicate pass, and exit 1 to indicate failure
cat >input.tcl <<EOF
proc bgerror x {
  puts \$x
  exit 1
}

source $here/test/assert.tcl
proc afterMinskyStarted {} {uplevel #0 {
 set id [newPlot]
 plot.get \$id
 foreach opt {none left right} {
   set plotWindowOptions_legend \$opt
   plot::setLegend 
 }

plot::resize  \$id
plot::resizeRect plotBBox [expr [plot.x]+100] [expr [plot.y]+100] 
set x [expr [plot.x]-[.wiring.canvas canvasx 0]]
set y [expr [plot.y]-[.wiring.canvas canvasy 0]]
plot::resizeItem plotBBox \$id [expr \$x+100] [expr \$y+100] 
assert {abs(200-[plot.width])<2}
assert {abs(200-[plot.height])<2}

tcl_exit

}}
EOF

$here/gui-tk/minsky input.tcl
if test $? -ne 0; then fail; fi

pass
