__kernel void convolute(const __global int * image ,__global int * newImg,__global int * G,int W,int H) 
{ 
	
	int i=get_global_id(0);//id del thread

	unsigned int x,y,imgLineSize;
	float value;//accumulatore
	int xOff,yOff,center;

	int size=3;//dimensione del kernel
   
	imgLineSize = W*3;
	center = (size-1)/2;//centro del kernel
	
	if(i >= imgLineSize*(size-center)+center*3 &&i < W*H*3-imgLineSize*(size-center)-center*3){
		value=0;
		for(y=0;y<size;y++){
			yOff = imgLineSize*(y-center);
            for(x=0;x<size;x++)
            {
				xOff = 3*(x-center);
                value += G[y*size+x]*image[i+xOff+yOff];
            }
        }
        newImg[i] = value;//setto l'accumulatore uguale al canale analizzato
	}
	
	
	
 }
