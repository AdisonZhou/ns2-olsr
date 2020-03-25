
BEGIN {
	init=0;
	i=0;
}
{
	event = $1;
	time = $2;
	node = $3;
	trace_type = $4;
	flag = $5;
	uid = $6;
	pkt_type = $7;
	pkt_size = $8;
    send_adr=$24;
    recv_adr=$25;

	if(event=="r" && node=="_31_" && pkt_type=="cbr" && send_adr=="[30:0" && recv_adr=="31:0") {
	   pkt_byte_sum[i+1]=pkt_byte_sum[i]+(pkt_size-20);
			if(init==0)
			{
				start_time=time;
				init=1;
			}
			end_time[i]=time;
			i++;
		}
}
END {
	th = 8*pkt_byte_sum[i-1]/(end_time[i-1]-start_time)/1000;
	printf("%f %.3f\n", rate, th);
}


