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
set val(rp)         OLSR                       ;# 路由协议
set val(x)          1000                        ;# 拓扑－长度
set val(y)          1000                        ;# 拓扑－宽度
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
set tracefd [open olsr_test.tr w]
set namtrace [open olsr_test.nam w]
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
set n(0) [$ns node] 
#$n(0) shape hexagon
#$n(0) label n0
#$n(0) label-color Red
$n(0) random-motion 0
$n(0) set X_ 50.0
$n(0) set Y_ 100.0
$n(0) set Z_ 0.0
$ns initial_node_pos $n(0) 20


set n(1) [$ns node]
$n(1) random-motion 0
$n(1) set X_ 200.0
$n(1) set Y_ 100.0
$n(1) set Z_ 0.0
$ns initial_node_pos $n(1) 20

set n(2) [$ns node]
$n(2) random-motion 0
$n(2) set X_ 350.0
$n(2) set Y_ 100.0
$n(2) set Z_ 0.0
$ns initial_node_pos $n(2) 20 

set n(3) [$ns node]
$n(3) random-motion 0
$n(3) set X_ 500.0
$n(3) set Y_ 100.0
$n(3) set Z_ 0.0
$ns initial_node_pos $n(3) 20 

set n(4) [$ns node]
$n(4) random-motion 0
$n(4) set X_ 650.0
$n(4) set Y_ 100.0
$n(4) set Z_ 0.0
$ns initial_node_pos $n(4) 20 

set n(5) [$ns node]
$n(5) random-motion 0
$n(5) set X_ 800.0
$n(5) set Y_ 100.0
$n(5) set Z_ 0.0
$ns initial_node_pos $n(5) 20 



set n(6) [$ns node] 
$n(6) random-motion 0
$n(6) set X_ 50.0
$n(6) set Y_ 250.0
$n(6) set Z_ 0.0
$ns initial_node_pos $n(6) 20


set n(7) [$ns node]
$n(7) random-motion 0
$n(7) set X_ 200.0
$n(7) set Y_ 250.0
$n(7) set Z_ 0.0
$ns initial_node_pos $n(7) 20

set n(8) [$ns node]
$n(8) random-motion 0
$n(8) set X_ 350.0
$n(8) set Y_ 250.0
$n(8) set Z_ 0.0
$ns initial_node_pos $n(8) 20 

set n(9) [$ns node]
$n(9) random-motion 0
$n(9) set X_ 500.0
$n(9) set Y_ 250.0
$n(9) set Z_ 0.0
$ns initial_node_pos $n(9) 20 

set n(10) [$ns node]
$n(10) random-motion 0
$n(10) set X_ 650.0
$n(10) set Y_ 250.0
$n(10) set Z_ 0.0
$ns initial_node_pos $n(10) 20 

set n(11) [$ns node]
$n(11) random-motion 0
$n(11) set X_ 800.0
$n(11) set Y_ 250.0
$n(11) set Z_ 0.0
$ns initial_node_pos $n(11) 20 

for {set i 0} {$i<12} {incr i} {
set rt($i) [$n($i) agent 255]
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
#  v
# 仿真结束时重置节点
for {set i 0} {$i < 12 } {incr i} {
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


