BEGIN {
	highest_uid=0;
}
{
    event = $1;  #事件类型
    time = $2;   #发生时间
    node = $3;   #事件发生所在的节点
    trace_type = $4;     #无线第4字段为事件发生所在的网络层面
    flag = $5;       #流标识
    uid = $6;       #无线第6字段为分组的uid
    pkt_type = $7;   #无线第7字段为分组的类型
    pkt_size = $8;   #无线第8字段为分组的大小
    send_adr=$24;
    recv_adr=$25;
 #   k=0;
 #   total_delay=0;
    if(event=="s" &&  node=="_30_" && trace_type=="AGT" && pkt_type=="cbr" && send_adr=="[30:0" && recv_adr=="31:0")
   {
      start_time[uid] = time;
     # printf("send uid=%d\n",uid);
   }#s发送，源节点2，AGT，CBR 的发送时间 
    else if(event=="r" && node=="_31_" && pkt_type=="cbr" && trace_type=="AGT" && send_adr=="[30:0" && recv_adr=="31:0")
      {
         #total_delay=total_delay+time-start_time[uid];
        # k++;
    #  printf("recved    OOO uid=%d \n",uid);
         end_time[uid] = time;  #r收到，目的节点0，CBR 接收时间
         if(highest_uid < uid) 
       {
           highest_uid = uid;  #更新highest_uid的值
          # printf ("high_uid============%d",highest_uid);
        }
      }
			
}
END {
     #   printf("highest_uid=%d",highest_uid);
	id=1;
	k=0;
	total_delay=0;
	avg_delay=0;
	for(i=0; i<=highest_uid; i++ )
	{
		start = start_time[i];
		end = end_time[i];
		delay = end - start;   #计算分组延迟时间
		if(delay > 0) {
			total_delay=total_delay+delay;
			k++;
		}
	}
        if (k!=0){
	avg_delay=total_delay/k;
	printf("%f %.9f\n",rate,avg_delay);}
}

