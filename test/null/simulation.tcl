# ======================================================================
# Define options
# ======================================================================
set opt(chan)           Channel/WirelessChannel  ;# channel type
set opt(prop)           Propagation/TwoRayGround ;# radio-propagation model**
set opt(netif)          Phy/WirelessPhy          ;# network interface type
set opt(mac)            Mac/802_11               ;# MAC type
set opt(ifq)            Queue/DropTail/PriQueue  ;# interface queue type
set opt(ll)             LL                       ;# link layer type
set opt(ant)            Antenna/OmniAntenna      ;# antenna model
set opt(ifqlen)         50                       ;# max packet in ifq
set opt(nn)             50                       ;# number of mobilenodes
set opt(adhocRouting)   OLSR                     ;# routing protocol
set opt(cp)             "cbr-50-20-2"               ;# connection pattern file
set opt(sc)             "scen-50-0-20-20-1000*400"   ;# node movement file. 
set opt(tr)		example2.tr          ;# trace file
set opt(nam)            example2.nam	 ;# nam trace file
set opt(x)              1000                      ;# x coordinate of topology
set opt(y)              400                      ;# y coordinate of topology
set opt(seed)           1.0                      ;# seed for random number gen.
set opt(stop)           20.0                     ;# time to stop simulation
#set opt(lm)             "off"           	 ;# log movement #node-config movementTrace option
set opt(energymodel)    EnergyModel    ;
#set opt(energymodel)   RadioModel     ;
set opt(radiomodel)    	RadioModel     ;
set opt(initialenergy)  10000.0           ;# Initial energy in Joules
#set opt(logenergy)      "on"          ;# log energy every 150 seconds
set opt(cbr-start)      5.0
set opt(txPower) 0.660 ;#传输能量
set opt(rxPower) 0.395 ;#接受能量
set opt(idlePower) 0.035 ;#待机能量
# ============================================================================
# Other default settings
$opt(mac) set basicRate_ 6Mb  ;# set this to 0 if want to use bandwidth_ for 
$opt(mac) set dataRate_ 54Mb   ;# both control and data pkts

LL set mindelay_		50us
LL set delay_			25us
LL set bandwidth_		0	;# not used

Agent/Null set sport_		0
Agent/Null set dport_		0

Agent/CBR set sport_		0
Agent/CBR set dport_		0

Agent/TCPSink set sport_	0
Agent/TCPSink set dport_	0

Agent/TCP set sport_		0
Agent/TCP set dport_		0
Agent/TCP set packetSize_	1460

Queue/DropTail/PriQueue set Prefer_Routing_Protocols    1

# unity gain, omni-directional antennas
# set up the antennas to be centered in the node and 1.5 meters above it
Antenna/OmniAntenna set X_ 0
Antenna/OmniAntenna set Y_ 0
Antenna/OmniAntenna set Z_ 1.5
Antenna/OmniAntenna set Gt_ 1.0
Antenna/OmniAntenna set Gr_ 1.0

# Initialize the SharedMedia interface with parameters to make
# it work like the 914MHz Lucent WaveLAN DSSS radio interface
Phy/WirelessPhy set CPThresh_ 10.0		;#capture threshold (db)
Phy/WirelessPhy set CSThresh_ 1.559e-11         ;#carrier sense threshold (W)
# below is original
#Phy/WirelessPhy set RXThresh_ 3.652e-10
# 95% pkts can be correctly received at 20m for 3/5.
Phy/WirelessPhy set RXThresh_ 3.652e-10		;#receive power threshold (w) for range specification of a node 
Phy/WirelessPhy set Rb_ 2*1e6
Phy/WirelessPhy set Pt_ 0.2818                  ;#transmitted signal power (W) For 250m transmission range.
Phy/WirelessPhy set freq_ 914e+6                ;#frequency
Phy/WirelessPhy set L_ 1.0                      ;#system loss factor

# Pt_ is transmitted signal power. The propagation model and Pt_ determines
# the received signal power of each packet. The packet can not be correctly
# received if received power is below RXThresh_.

# ======================================================================
# Main Program
# ======================================================================

#
# check for random seed
#
if {$opt(seed) > 0} {
    puts "Seeding Random number generator with $opt(seed)\n"
    ns-random $opt(seed)
}

#
# create simulator instance
#
set ns_ [new Simulator]

#
# control OLSR behaviour from this script -
# commented lines are not needed because
# those are default values
#
Agent/OLSR set use_mac_    true
#Agent/OLSR set debug_      false
#Agent/OLSR set willingness 3
#Agent/OLSR set hello_ival_ 2
#Agent/OLSR set tc_ival_    5

#
# open traces
#
set tracefd  [open $opt(tr) w]
set namtrace [open $opt(nam) w]
set prop     [new $opt(prop)]

# use new trace file format

# $ns_ use-newtrace 

$ns_ trace-all $tracefd
$ns_ namtrace-all-wireless $namtrace $opt(x) $opt(y)

#
# create topography object
#
set topo [new Topography]

#
# define topology
#
$topo load_flatgrid $opt(x) $opt(y)

#
# create God
#
#other format is set god_ [create-god $opt(nn)] 
set god_ [create-god $opt(nn)]

#
# configure mobile nodes
#

# Create channel #1
set chan_1_ [new $opt(chan)]

#global node setting

$ns_ node-config -adhocRouting $opt(adhocRouting) \
                 -llType $opt(ll) \
                 -macType $opt(mac) \
                 -ifqType $opt(ifq) \
                 -ifqLen $opt(ifqlen) \
                 -antType $opt(ant) \
                 -propType $opt(prop) \
                 -phyType $opt(netif) \
		 -channel $chan_1_ \
                 -topoInstance $topo \
                 -wiredRouting OFF \
                 -agentTrace ON \
                 -routerTrace ON \
                 -macTrace ON \
		 -energyModel $opt(energymodel) \
		 -idlePower 0.0 \
		 -rxPower 1.0 \
		 -txPower 1.4 \
          	 -sleepPower 0.001 \
          	 -transitionPower 0.2 \
          	 -transitionTime 0.005 \
		 -initialEnergy $opt(initialenergy) \
                 -txPower $opt(txPower) \
                 -rxPower $opt(rxPower) \
                 -idlePower $opt(idlePower) \
		 -movementTrace OFF 

#-sleepPower: power consumption (Watt) in sleep state
#-transitionPower:power consumption (Watt) in state transition from sleep to idle (active)
#--transitionTime: time (second) used in state transition from sleep to idle (active)

$ns_ set WirelessNewTrace_ ON

for {set i 0} {$i < $opt(nn)} {incr i} {
    set node_($i) [$ns_ node]
    $node_($i) random-motion 0		;# disable random motion
#$node_($i) topography $topo
}

#
# positions not necessary for opt(sc)
#
#$node_(0) set X_ 350.0
#$node_(0) set Y_ 200.0
#$node_(0) set Z_ 0.0

#$node_(1) set X_ 200.0
#$node_(1) set Y_ 350.0
#$node_(1) set Z_ 0.0

#$node_(2) set X_ 200.0
#$node_(2) set Y_ 550.0
#$node_(2) set Z_ 0.0

#$node_(3) set X_ 50.0
#$node_(3) set Y_ 200.0
#$node_(3) set Z_ 0.0

#$node_(4) set X_ 200.0
#$node_(4) set Y_ 50.0
#$node_(4) set Z_ 0.0#

#
# setup UDP connection not necessary for opt(cp)
#
# 4 connecting to 6 at time 112.66623783515126
#
set udp_(1) [new Agent/UDP]
$ns_ attach-agent $node_(7) $udp_(1)
set null_(1) [new Agent/Null]
$ns_ attach-agent $node_(35) $null_(1)
set cbr_(1) [new Application/Traffic/CBR]
$cbr_(1) set packetSize_ 512
$cbr_(1) set interval_ 0.5
$cbr_(1) set random_ 1
$cbr_(1) set maxpkts_ 10000
$cbr_(1) attach-agent $udp_(1)
$ns_ connect $udp_(1) $null_(1)
$ns_ at 3.66623783515126 "$cbr_(1) start"
#

#
# print (in the trace file) routing table and other
# internal data structures on a per-node basis
#
$ns_ at 10.0 "[$node_(0) agent 255] print_rtable"
$ns_ at 15.0 "[$node_(0) agent 255] print_linkset"
$ns_ at 20.0 "[$node_(0) agent 255] print_nbset"
$ns_ at 25.0 "[$node_(0) agent 255] print_nb2hopset"
$ns_ at 30.0 "[$node_(0) agent 255] print_mprset"
$ns_ at 35.0 "[$node_(0) agent 255] print_mprselset"
$ns_ at 40.0 "[$node_(0) agent 255] print_topologyset"

#
# source connection-pattern and node-movement scripts
#
if { $opt(cp) == "" } {
    puts "*** NOTE: no connection pattern specified."
    set opt(cp) "none"
} else {
    puts "Loading connection pattern..."
    source $opt(cp)
}
if { $opt(sc) == "" } {
    puts "*** NOTE: no scenario file specified."
    set opt(sc) "none"
} else {
    puts "Loading scenario file..."
    source $opt(sc)
    puts "Load complete..."
}

#
# define initial node position in nam
#
for {set i 0} {$i < $opt(nn)} {incr i} {
    $ns_ initial_node_pos $node_($i) 20
}     

#
# tell all nodes when the simulation ends
#
for {set i 0} {$i < $opt(nn) } {incr i} {
    $ns_ at $opt(stop).0 "$node_($i) reset";
}

$ns_ at $opt(stop).0002 "puts \"NS EXITING...\" ; $ns_ halt"
$ns_ at $opt(stop).0001 "stop"

proc stop {} {
    global ns_ tracefd namtrace
    $ns_ flush-trace
    close $tracefd
    close $namtrace
}

#
# begin simulation
#
puts "Starting Simulation..."

$ns_ run
