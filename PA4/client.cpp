#include <sys/wait.h>
#include <thread>
#include "common.h"
#include "FIFOreqchannel.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "HistogramCollection.h"

using namespace std;

string filename = "";
/* Sameer Hussain - CSCE 313 - PA 4*/

bool isOutFile = false;
__int64_t amountReceived;
__int64_t totalAmount;

HistogramCollection hc;

void sigFunc(int sig)
{
	if(!filename.empty())
	{
		system("clear");
		hc.print();
	}
	else
	{
		system("clear");
		cout << "\n File Download Progress ... \n  " << amountReceived*100 / totalAmount << " % done..." << endl;
	}

	alarm(2);
}

struct Response // a struct to hold person and ecg number
{
	int pno;
	double ecg;
};

struct thread_data // this struct is for me to use in file_thread_function for appropriate attributes of a thread
{
	string filename;
	FIFORequestChannel* control;
	int m;
	BoundedBuffer* requestBuffer;
};

// write create_new_channel

void timediff (struct timeval& start, struct timeval& end){
    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;
}

FIFORequestChannel* create_new_channel (FIFORequestChannel* mainchan)
{
    char name [1024];
    Request m = NEWCHAN_REQ_TYPE;
    mainchan->cwrite (&m, sizeof (m));
    mainchan->cread (name, 1024);
    FIFORequestChannel* newchan = new FIFORequestChannel (name, FIFORequestChannel::CLIENT_SIDE);
    return newchan;
}

// tanzir
void patient_thread_function(int pno, int n, BoundedBuffer* requestBuffer)
{
    DataRequest d (pno, 0.0, 1);
	for (int i = 0; i<n; i++ ){
		vector<char> temp ((char*) &d, ((char*)&d + sizeof (d)));
		requestBuffer->push(temp);
		d.seconds += .004;
	}
}

// tanzir
void file_thread_function(void* info)
{

	thread_data* dat = (thread_data*)info;
	string filename = dat->filename;
	FIFORequestChannel* control = dat->	control;

	int m = dat->m;
	BoundedBuffer* requestBuffer = dat->	requestBuffer;


	string str_filename = "received/" + filename; // where the file goes
	FILE* fp = fopen(str_filename.c_str(), "w"); // write file
	fclose(fp);
	
	// fd(fp, str_filename); file descriptor

	int len_filemsg = sizeof(FileRequest) + sizeof(filename)+ 1;
	char fileMsgBuffer [len_filemsg];
	FileRequest* fm = new (fileMsgBuffer) FileRequest(0,0);
	strcpy (fileMsgBuffer + sizeof (FileRequest), filename.c_str());


	control->cwrite(fileMsgBuffer, sizeof(fm)+ sizeof(filename) + 1); 

	__int64_t fileSize;
	control->cread(&fileSize, sizeof(fileSize));

	__int64_t rem = fileSize; 
	char recvbuffer[m];
	FileRequest* to_send = (FileRequest*)fileMsgBuffer;
	totalAmount = rem;

	while (rem > 0)
	{
		to_send->length = min(rem, (__int64_t ) m);
		vector<char> temp (fileMsgBuffer, fileMsgBuffer + sizeof(FileRequest)+sizeof(filename)+1);
		requestBuffer -> push(temp);
		rem -= to_send->length;
		to_send->offset += to_send->length;
		amountReceived += fm->length;
	}
}

void worker_thread_function(BoundedBuffer* requestBuffer, FIFORequestChannel* chan, BoundedBuffer* resb, int buffercapacity)
{
	char resp[buffercapacity];
	while(true)
	{
		vector<char> req = requestBuffer->pop();
		char* request = req.data();

		chan->cwrite(request, req.size());

		Request* m = (Request*) request;


		if (m->getType() == DATA_REQ_TYPE)
		{
		
			int pno = ((DataRequest*) request) -> person;
			
			double ecg = 0;
			chan->cread(&ecg, sizeof(double));

			Response r {pno, ecg};
			vector<char> temp ((char*)&r, (char*)&r + sizeof(r));
			resb->push(temp);
		}
		else if(m->getType() == QUIT_REQ_TYPE)
		{
			break;
		}
		else if (m->getType() == FILE_REQ_TYPE)
		{
			chan->cread(resp, sizeof(resp));
			FileRequest* fm = (FileRequest*) request;
			string fname = fm->getFileName();

			fname = "received/" + fname;
			FILE* fp = fopen (fname.c_str(), "r+");
			fseek(fp, fm->offset, SEEK_SET);
			fwrite(resp,1, fm->length, fp);
			fclose(fp);
		}
	}
}

void histogram_thread_function (BoundedBuffer* resb, HistogramCollection* hc)
{
    while(true)
	{
		vector<char> resp = resb->pop();
		Response* r = (Response*) resp.data();
		if (r->pno == -1)
		{
			break;
		}
		hc -> update(r->pno, r->ecg);	
	}
}


int main(int argc, char *argv[])
{
	signal(SIGALRM, sigFunc);
	alarm(2);
	

	int n = 10000;
	int p = 10;
	int w = 100;
	int b = 1024;
	int h = 3;
	double t = -1.0;
	
	int m = MAX_MESSAGE;

	Request qm (QUIT_REQ_TYPE); // quit message for closing channels

	srand(time_t(NULL));
	string capacity_str = "";

	int opt;
	while ((opt = getopt(argc, argv, "n:p:t:w:b:m:f:h:")) != -1) 
	{
		switch (opt) 
		{
			case 'p':
				p = atoi(optarg);
				break;
			case 't':
                t = atof (optarg);
                break;
			case 'n':
				n = atoi(optarg);
				break;
			case 'w':
				w = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
				break;
			case 'm':
				m = atoi(optarg);
				break;
			case 'h':
				h = atoi(optarg);
				break;
			case 'f':
				filename = optarg;
				break;
		}
	}

	capacity_str = to_string(m);

	int pid = fork ();
	// int pid = 0;
	if (pid < 0)
	{
		EXITONERROR ("Could not create a child process for running the server");
	}
	if (!pid){ // The server runs in the child process
		char* args[] = {"./server", "-m", (char*) to_string(m).c_str(), nullptr};
		if (execvp(args[0], args) < 0){
			EXITONERROR ("Could not launch the server");
		}
	}


	FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);
	BoundedBuffer request_buffer(b);
	BoundedBuffer response_buffer(b);

	struct timeval start, end;
    gettimeofday (&start, 0);


	//W channels - 1 for each worker
	FIFORequestChannel* wchans [w];
	for (int i = 0; i < w; i++)
	{
		Request nc (NEWCHAN_REQ_TYPE);
		chan.cwrite (&nc, sizeof(nc));
		char cname [1024];
		chan.cread(cname, sizeof(cname));
		// create_new_channel()
		wchans [i] = new FIFORequestChannel (cname, FIFORequestChannel::CLIENT_SIDE);

	}

	thread workers [w];
	for (int i = 0; i < w; i++)
	{
		workers [i] = thread (worker_thread_function, &request_buffer, wchans[i], &response_buffer, m);
	}


	thread patents[p];
	thread histhreads [h];
	thread filethread;

	
	if (filename.empty())
	{

		for(int i = 0; i < p; i++) {
			Histogram* h = new Histogram(10,-2.0, 2.0);
			hc.add(h);
		}

		thread patients [p];
		for (int i = 0; i < p; i++)
		{
			patients[i] = thread (patient_thread_function, i+1, n, &request_buffer);
		}

		for (int i = 0; i < h; i++) 
		{ 
			histhreads[i] = thread (histogram_thread_function, &response_buffer, &hc);
		}

		//Waiting for threads
		for (int i = 0; i < p; i++)
		{
			patients[i].join();
		}

		
		vector<char> temp ((char*) &qm, ((char*)&qm + sizeof (qm)));
		for (int i = 0; i < w; i++)
		{
			request_buffer.push(temp);
		}
		
		//workers

		for (int i = 0; i < w; i++){
			workers[i].join();
		}

		Response rq {-1, 0.0};
		vector<char> t((char*) &rq, (char*) &rq+sizeof (rq));
		for (int i = 0; i < h; i++){
			response_buffer.push(t);
		}

		for (int i = 0; i < h; i++){
			histhreads[i].join();
		}

		hc.print();
	}
	else 
	{

		thread_data dat = {filename, &chan, m, &request_buffer};
		filethread = thread (file_thread_function, (void*)&dat);
		filethread.join();
		vector<char> temp ((char*) &qm, ((char*)&qm + sizeof (qm)));

		for (int i = 0; i < w; i++)
		{
			request_buffer.push(temp);
		}

		for (int i = 0; i < w; i++)
		{
			workers[i].join();
		}
		
	}

    gettimeofday (&end, 0);
	
	timediff(start, end);
	
	vector<char> temp ((char*)&qm, (char*)&qm + sizeof (qm));
    chan.cwrite (&qm, sizeof (Request));

	for (int i = 0; i < w; i++)
	{
		delete wchans[i];
	}

	wait(0);
	cout << "Client process exited" << endl;

}
