#简单无线网络模型模拟

#无线节点参数
set val(chan)       Channel/WirelessChannel    ;# channel type 信道类型：无线信道
set val(prop)       Propagation/TwoRayGround   ;# radio-propagation model 信道模型：TwoRayGround
set val(netif)      Phy/WirelessPhy            ;# network interface type 无线物理层
set val(mac)        Mac/802_11                 ;# MAC type MAC层协议
set val(ifq)        Queue/DropTail/PriQueue    ;# interface queue type
set val(ll)         LL                         ;# link layer type 
set val(ant)        Antenna/OmniAntenna        ;# antenna model
set val(ifqlen)     50                         ;# max packet in ifq
set val(nn)             50                       ;# number of mobilenodes
set val(rp)         AOLSR                       ;# 路由协议
set val(x)          800                        ;# 拓扑－长度
set val(y)          800                        ;# 拓扑－宽度
set val(sc)             "scen"   ;# node movement file. 
set val(stop)       30.0 					        ;# time of simulation end
set val(engmodel) EnergyModel ;#能量模型
set val(initeng) 10000.0 ;#总能量
set val(txPower) 0.660 ;#传输能量
set val(rxPower) 0.395 ;#接受能量
set val(idlePower) 0.035 ;#待机能量


# 建立一个simulator实例
set ns [new Simulator]
#$ns use-newtrace

#开启Trace跟踪和NAM跟踪
set tracefd [open aolsr_test.tr w]
set namtrace [open aolsr_test.nam w]
$ns trace-all $tracefd
$ns namtrace-all-wireless $namtrace $val(x) $val(y)

#建立topology对象
set topo [new Topography]
$topo load_flatgrid $val(x) $val(y)


#创建god
create-god 12

set chan_1_ [new $val(chan)]

#配置无线节点（包括使用何种路由协议，何种mac协议，无线信道的模型等等）
$ns node-config -adhocRouting $val(rp) \
                -llType $val(ll) \
                -macType $val(mac) \
                -ifqType $val(ifq) \
                -ifqLen $val(ifqlen) \
                -antType $val(ant) \
                -propType $val(prop)    \
                -phyType $val(netif) \
					 -channel $chan_1_  \
                -topoInstance $topo \
                -agentTrace ON \
                -routerTrace ON \
                -macTrace ON    \
                -movementTrace OFF \
                -energyModel $val(engmodel) \
                -initialEnergy $val(initeng) \
                -txPower $val(txPower) \
                -rxPower $val(rxPower) \
                -idlePower $val(idlePower) 

Agent/AOLSR set use_mac_    true
#建立无线节点并设置节点的位置（节点位置决定了拓扑结构）
for {set i 0} {$i < $val(nn)} {incr i} {
    set node_($i) [$ns_ node]
    $node_($i) random-motion 0	
    set rt($i) [$n($i) agent 255]
    $rt($i) cl-mac [$n($i) set mac_(0)]
}
#
set udp_(1) [new Agent/UDP]
$ns attach-agent $n(0) $udp_(1)
set null_(1) [new Agent/Null]
$ns attach-agent $n(11) $null_(1)
set cbr(1) [new Application/Traffic/CBR]
$cbr(1) set packetSize_ 512
$cbr(1) set interval_ 0.05
$cbr(1) set random_ 1
$cbr(1) set maxpkts_ 10000
$cbr(1) attach-agent $udp_(1)
$ns connect $udp_(1) $null_(1)
$ns at 3.66623783515126 "$cbr(1) start"
#    

#
set udp_(2) [new Agent/UDP]
$ns attach-agent $n(5) $udp_(2)
set null_(2) [new Agent/Null]
$ns attach-agent $n(10) $null_(2)
set cbr(2) [new Application/Traffic/CBR]
$cbr(2) set packetSize_ 512
$cbr(2) set interval_ 0.05
$cbr(2) set random_ 1
$cbr(2) set maxpkts_ 10000
$cbr(2) attach-agent $udp_(2)
$ns connect $udp_(2) $null_(2)
$ns at 3.65623783515126 "$cbr(2) start"
#        

#
set udp_(3) [new Agent/UDP]
$ns attach-agent $n(1) $udp_(3)
set null_(3) [new Agent/Null]
$ns attach-agent $n(8) $null_(3)
set cbr(3) [new Application/Traffic/CBR]
$cbr(3) set packetSize_ 512
$cbr(3) set interval_ 0.05
$cbr(3) set random_ 1
$cbr(3) set maxpkts_ 10000
$cbr(3) attach-agent $udp_(3)
$ns connect $udp_(3) $null_(3)
$ns at 3.64623783515126 "$cbr(3) start"
#

if { $val(cp) == "" } {
    puts "*** NOTE: no connection pattern specified."
    set val(cp) "none"
} else {
    puts "Loading connection pattern..."
    source $val(cp)
}
if { $opt(sc) == "" } {
    puts "*** NOTE: no scenario file specified."
    set opt(sc) "none"
} else {
    puts "Loading scenario file..."
    source $val(sc)
    puts "Load complete..."
}

#
# define initial node position in nam
#
for {set i 0} {$i < $val(nn)} {incr i} {
    $ns_ initial_node_pos $node_($i) 20
}     


# 仿真结束时重置节点
for {set i 0} {$i < 30 } {incr i} {
	$ns at 30.0 "$n($i) reset";
}

#启动和结束流代理


#定义结束进程
proc finish {} {
   	global ns tracefd namtrace
   	$ns flush-trace
   	close $tracefd
   	close $namtrace
	exit 0
}

#仿真结束时调用结束进程
$ns at $val(stop) "finish"
$ns at $val(stop) "puts \"NS EXISTING...\"; $ns halt"


puts "Start Simulation..."

# run the simulation
$ns run


