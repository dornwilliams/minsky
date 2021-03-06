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
source $here/test/assert.tcl
proc afterMinskyStarted {} {uplevel #0 {
 minsky.load $here/examples/GoodwinLinear02.mky
 openGlobalInCanvas
 recentreCanvas
 update

foreach gid [items.#keys] {
   item.get \$gid
  if {[item.classType]=="GroupIcon"} break
 }
 #create an op, and move it into the group
 group.get \$gid
 group.detailedText "the group"
 item.get [addOperation time]
 item.detailedText "addedTimeOp"

 event generate .wiring.canvas <Button-1> -x [group.x]  -y [group.y]

 # do the same thing with a variable
 
 set id [findItemWithDetailedText "addedTimeOp"]
 set gid [findItemWithDetailedText "the group"]
 op.get \$id
 assert "\[groupOf \$id\]==\$gid" "op"

 move \$id 1000 1000
 update
 checkAddGroup \$id  1000 1000
 set id [findItemWithDetailedText "addedTimeOp"]
 set gid [findItemWithDetailedText "the group"]
 assert "\[groupOf \$id\]!=\$gid" "op1"

 foreach id [items.#keys] {
   item.get \$id
   switch -glob [item.classType] {
     "Variable*" {
       if [item.visible] break
     }
   }
 }
 copyVar
 var.detailedText "copied var"
 event generate .wiring.canvas <Button-1> -x [group.x]  -y [group.y]
 set varId [findItemWithDetailedText "copied var"]
 set gid [findItemWithDetailedText "the group"]
 assert "\[groupOf  \$varId]==\$gid" "var"

 move \$varId 1000 1000
 update
 checkAddGroup \$varId  1000 1000
 set varId [findItemWithDetailedText "copied var"]
 set gid [findItemWithDetailedText "the group"]
 assert "\[groupOf \$varId\]!=\$gid" "var"

 set x [group.x]
 set y [group.y]

 set newGroupId [insertGroupFromFile $here/examples/GoodwinLinear02.mky]
 insertNewGroup \$newGroupId
 group.get \$newGroupId
 group.detailedText "inserted group from file"

 event generate .wiring.canvas <Button-1> -x \$x  -y \$y
 set newGroupId [findItemWithDetailedText "inserted group from file"]
 set gid [findItemWithDetailedText "the group"]
 assert "\[groupOf \$newGroupId\]==\$gid" "group"

 move \$newGroupId 1000 1000
 checkAddGroup \$newGroupId  1000 1000
 set newGroupId [findItemWithDetailedText "inserted group from file"]
 set gid [findItemWithDetailedText "the group"]
 assert "\[groupOf \$newGroupId\]!=\$gid" "group"

 group.get \$gid
 set numGroups [group.groups.size]
 group::copy \$gid
 event generate .wiring.canvas <Button-1> -x \$x  -y \$y
 set gid [findItemWithDetailedText "the group"]
 group.get \$gid
 assert "\$numGroups==[expr [group.groups.size]-1]"

 groupEdit \$gid

 update

 assert {[winfo viewable .wiring.editGroup]}
 .wiring.editGroup.buttonBar.ok invoke
 assert {![winfo exists .wiring.editGroup]}

 tcl_exit
}}
EOF

$here/gui-tk/minsky input.tcl
if test $? -ne 0; then fail; fi

pass
