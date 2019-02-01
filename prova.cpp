#include <iostream>
#include <vector>
#include <assert.h>
#include <cmath>

#include <png++/png.hpp>

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl.h>

#define MAX_SOURCE_SIZE (0x100000)
using namespace std; 

typedef vector<int> Array;
typedef vector<Array> Matrix;//variabili necessarie per contenere l'immagine
typedef vector<Matrix> Image;


Image loadImage(const char *filename)//funzione per caricare l'immagine, richiede come parametro il nome
{
    png::image<png::rgb_pixel> image(filename);
    Image imageMatrix(3, Matrix(image.get_height(), Array(image.get_width())));
    //dichiaro la matrice che rappresenta l 'immagine
    int h,w;
    for (h=0 ; h<image.get_height() ; h++) {
        for (w=0 ; w<image.get_width() ; w++) {
            imageMatrix[0][h][w] = image[h][w].red;
            imageMatrix[1][h][w] = image[h][w].green;
            imageMatrix[2][h][w] = image[h][w].blue;
        }
    }

    return imageMatrix;//ritorno l'immagine come matrice
}

void saveImage(Image &image, const char *filename)//salvol'immagine
{
    assert(image.size()==3);

    int height = image[0].size();//prendo l' altezza
    int width = image[0][0].size();//prendo la larghezza
    int x,y;//creo due variabili di scorrimento

    png::image<png::rgb_pixel> imageFile(width, height);
	//prima di salvare effettivamente l'immagine, controllo che i valori siano nel range per quali sono stati pensati
    for (y=0 ; y<height ; y++) {
        for (x=0 ; x<width ; x++) {
		if(image[0][y][x]>=255){
			image[0][y][x]=255;
		}
		if(image[1][y][x]>=255){
			image[1][y][x]=255;
		}
		if(image[2][y][x]>=255){
			image[2][y][x]=255;
		}
		if(image[0][y][x]<=0){
			image[0][y][x]=0;
		}
		if(image[1][y][x]<=0){
			image[1][y][x]=0;
		}
		if(image[2][y][x]<=0){
			image[2][y][x]=0;
		}
	    imageFile[y][x].red = image[0][y][x];//scrivo i vari canali dopo il controllo
            imageFile[y][x].green = image[1][y][x];
            imageFile[y][x].blue = image[2][y][x];
        }
    }
    imageFile.write(filename);;//una volta che ho controllato i pixel e li
//ho assegnati alla nuova immagine "imageFile", la salvo, con il
//nome "flename" specifcato dal parametro della funzione
}





int main()
{	
   	int n=3;//n è la size del filtro, ovviamente essendo una matrice, il filtro avra dimensione n*n
   	int filtro[3][3]={{-1,-1,-1},
			  {-1,8,-1},
                    	  {-1,-1,-1}};

	/*int filtro[3][3]={{0,0,0},
			  {0,1,0},
                    	  {0,0,0}};*/
   	int filtro2[9];//filtro 2 sarebbe il filtro visto come array da passare ad opencl
	
   	int tempa=0;//copio i valori del filtro nel filtro per il parallelo
   	for(int i=0;i<n;i++){
       		for(int j=0;j<n;j++){
           	filtro2[tempa]=filtro[i][j];	
           	tempa++;
       		}
   	}

    

    	printf("\nCarico l'immagine \n");

    	Image image = loadImage("6454.png");//nome del file che deve essere caricato, prima o poi lo metto da char argv per matchare con la relazione gia scritta

    	printf("Faccio la convoluzione l'immagine \n");

    	int height = image[0].size();//prendo l'altezza e la larghezza dell'immagine in input
    	int width = image[0][0].size();

    	int heightpar = height;//altezza della nuova immagine
    	int widthpar = width;//larghezza della nuova immagine

    	int filterHeight = 3;//altezza del kernel
    	int filterWidth = 3;//larghezza del kernel
    	int newImageHeight = height-filterHeight+1;//nuova altezza dell'immagine risultante
    	int newImageWidth = width-filterWidth+1;//nuova larghezza dell'immagine risultante
    	int d,i,j,h,w;

    	Image newImage(3, Matrix(newImageHeight, Array(newImageWidth)));//immagine modifica sequenziale
//nuova matrice rappresentante l'immagine che ospita i valori modificati
	clock_t start,end;
	double tempo;
	start=clock();
	
   	//faccio la convoluzione in sequenziale
        for (i=0 ; i<newImageHeight ; i++) {
            for (j=0 ; j<newImageWidth ; j++) {
                for (h=i ; h<i+filterHeight ; h++) {
                    for (w=j ; w<j+filterWidth ; w++) {
                        newImage[0][i][j] += filtro[h-i][w-j]*image[0][h][w];//valore r
			newImage[1][i][j] += filtro[h-i][w-j]*image[1][h][w];//valore g
			newImage[2][i][j] += filtro[h-i][w-j]*image[2][h][w];//valore b
                    }
                }
            }
        }
    	end=clock();
	tempo=((double)(end-start))/CLOCKS_PER_SEC;//faccio la differenza e vedo quanti secondi sono passati

	printf("TEMPO MODIFICA SEQUENZIALE =%f secondi\n",tempo);

    	printf("Salvo l'immagine \n");
    	saveImage(newImage, "sequenziale.png");//salvo la nuova immagine modificata con il nome sequenziale.png
	sleep(5);


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    printf("\nfinito, inizio la modifica parallela\n");
	Image newImage2(3, Matrix(heightpar, Array(widthpar)));//immagine modifica parallela, la dichiaro

	int *myarray = (int*)malloc(sizeof(int)*width*height*3);//conterrà l'immagine da modificare

	int *myarrayout = (int*)malloc(sizeof(int)*width*height*3);//conterrà l'immagine modificata in parallelo

	clock_t start1,end1;
	double tempo1;
	start1=clock();


	int temp=0;
	for(int i=0;i<height;i++){//copio i valori rgb dell'imagine nell'array myarray
		for(j=0;j<width;j++){
		myarray[temp]=image[0][i][j];
		temp++;
		myarray[temp]=image[1][i][j];
		temp++;
		myarray[temp]=image[2][i][j];
		temp++;
		}
	}
	//myarray contiene i valori rgbrgb

	
	int contatore=0;
	for(int s=0;s<width*height*3;s++){
		//printf("%d \n",myarray[s]);
		myarrayout[s]=0;
		contatore++; 
	}
	printf("nell'array my array ho num_pixel= %d \n\n",contatore);

	//debug...

	


	FILE *kernelFile;//VARIABILI CHE CONTERRANNO IL KERNEL
	char *kernelSource;
	size_t kernelSize;

	kernelFile = fopen("kernel.cl", "r");//APRO IL FILE CON IL KERNEL

	if (!kernelFile) {

		fprintf(stderr, "Non ho trovato nessun file chiamato kernel.cl\n");

		return 0;//termino il programma se non trovo il kernel

	}
	kernelSource = (char*)malloc(MAX_SOURCE_SIZE);
	kernelSize = fread(kernelSource, 1, MAX_SOURCE_SIZE, kernelFile);
	fclose(kernelFile);//chiudo il file

	//una volta definite le variabili con il kernel letto, chiudo il file
	// Prendo informazioni sulle piattaforme e i devices
	cl_platform_id platformId = NULL;
	cl_device_id deviceID = NULL;
	cl_uint retNumDevices;
	cl_uint retNumPlatforms;
	cl_int ret = clGetPlatformIDs(1, &platformId, &retNumPlatforms);
	ret = clGetDeviceIDs(platformId, CL_DEVICE_TYPE_DEFAULT, 1, &deviceID, &retNumDevices);

	// Creo il contesto
	cl_context context = clCreateContext(NULL, 1, &deviceID, NULL, NULL,  &ret);

	// Creo la coda dei comandi
	cl_command_queue commandQueue = clCreateCommandQueue(context, deviceID, 0, &ret);


	// creo i buffer per ogni array
	cl_mem aMemObj = clCreateBuffer(context, CL_MEM_READ_ONLY, width * height * 3 * sizeof(int), NULL, &ret);
	cl_mem bMemObj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, width * height * 3 * sizeof(int), NULL, &ret);
	cl_mem filtroMemObj = clCreateBuffer(context, CL_MEM_READ_ONLY, n*n* sizeof(int), NULL, &ret);
	

	// copio le liste ai buffer definiti in precedenza
	ret = clEnqueueWriteBuffer(commandQueue, aMemObj, CL_TRUE, 0, width * height * 3 * sizeof(int), myarray, 0, NULL, NULL);
	ret = clEnqueueWriteBuffer(commandQueue, bMemObj, CL_TRUE, 0, width * height * 3 * sizeof(int), myarrayout, 0, NULL, NULL);
	ret = clEnqueueWriteBuffer(commandQueue, filtroMemObj, CL_TRUE, 0, n*n* sizeof(int), filtro2, 0, NULL, NULL);
	
	
	// creo il programma dal codice del kernel
	cl_program program = clCreateProgramWithSource(context, 1, (const char **)&kernelSource, (const size_t *)&kernelSize, &ret);	

	// compilo il programma
	ret = clBuildProgram(program, 1, &deviceID, NULL, NULL, NULL);

	// Creo l'oggetto kernel
	cl_kernel kernel = clCreateKernel(program, "convolute", &ret);


	// Setto gli argomenti del kernel
	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&aMemObj);	
	ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&bMemObj);
	ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&filtroMemObj);
	ret = clSetKernelArg(kernel, 3, sizeof(int), (void *)&width);
	ret = clSetKernelArg(kernel, 4, sizeof(int), (void *)&height);


	size_t globalItemSize = width*height*3;
	size_t localItemSize = 1;

	//chiamiamo l'esecuzione del kernel
	
	ret = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &globalItemSize, NULL, 0, NULL, NULL);	



	//prendiamo i risultati dal device all'host
	ret = clEnqueueReadBuffer(commandQueue, bMemObj, CL_TRUE, 0, width * height * 3 * sizeof(int), myarrayout, 0, NULL, NULL);

		

	ret = clFlush(commandQueue);//libero le variabili e i buffer di memoria che ho allocato precedentemente
	ret = clFinish(commandQueue);
	ret = clReleaseCommandQueue(commandQueue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(aMemObj);
	ret = clReleaseMemObject(bMemObj);
	ret = clReleaseContext(context);

	
//da qui in poi mi serve per il salvataggio////////////////////////////////////////////////////////////////////////////////////////////////////////

	/*for(int m=0;m<width * height * 3;m++){
	//printf("%d \n",myarrayout[m]);
	}*/

	int conta=0;//prima di salvare l'immagine, devo riportare i valori in myarrayout in formato matrice per essere salvati
	for(int i=0;i<height;i++){
		for(j=0;j<width;j++){
		newImage2[0][i][j]=myarrayout[conta];
		conta++;
		newImage2[1][i][j]=myarrayout[conta];
		conta++;
		newImage2[2][i][j]=myarrayout[conta];
		conta++;
		}
	}

	end1=clock();
	tempo1=((double)(end1-start1))/CLOCKS_PER_SEC;

	printf("TEMPO MODIFICA PARALLELA =%f secondi\n",tempo1);

	printf("Salvo l'immagine parallela \n");
	sleep(5);
    	saveImage(newImage2, "parallela.png");//una volta fatta l'operazione, salvo la nuova immagine

return 0;

}
