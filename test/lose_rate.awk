BEGIN {
	fsDrops=0;
	numfs2=0;
	numfs0=0;
}
{
	event = $1;
	time = $2;
	node = $3;
	len = length(node);
        trace_type = $4;
	flag = $5;
	uid = $6;
	pkt_type = $7;
	pkt_size = $8;
        send_adr=$24;
        recv_adr=$25;
	if(event=="s" &&  node=="_30_" && trace_type=="AGT" && pkt_type=="cbr" && send_adr=="[30:0" && recv_adr=="31:0")
        {
               
			numfs2++;
	}
        if(event=="r" && node=="_31_" && trace_type=="AGT" && pkt_type=="cbr" && send_adr=="[30:0" && recv_adr=="31:0")
        {
			numfs0++;
	}
}
END {
	loss_rate=0;
	fsDrops=numfs2-numfs0;
	loss_rate=fsDrops/numfs2;
	printf("%f %.3f\n",rate,loss_rate);
}

