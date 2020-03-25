BEGIN {
	i=0;
	all=0;
for (i=0;i<38;i++)
temp2[j]=0;
}
{
	temp = $2;    
        j=i%38;
        temp2[j]=temp2[j]+temp;
        i=i+1;
}
END {
	for (k=0;k<38;k++)
	printf("%.6f\n",temp2[k]/20);
}

