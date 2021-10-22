#include "common.h"
#include <algorithm>
#include "FIFOreqchannel.h"
#include <sys/wait.h>
#include <chrono>
using namespace std;
using namespace std::chrono;

int main(int argc, char *argv[]){

	int opt;
	int p = 1;
	double t = -0.1;
	int e = -1;
	string filename = "";
	int buffercapacity = MAX_MESSAGE;
	string bcapacitystr = "";
	bool isNewChan = false;

	auto nc_st_dp = high_resolution_clock::now();


	// take all the arguments first because some of these may go to the server
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) 
	{
		switch (opt) 
		{
			case 'f':
				filename = optarg;
				break;
			case 'p':
				p = atoi(optarg);
				break;
			case 't':
				t = atof(optarg);
				break;
			case 'e':
				e = atoi(optarg);
				break;
			case 'm':
				bcapacitystr = optarg;
				buffercapacity = atoi(optarg);
				break;
			case 'c':
				isNewChan = true;
				break;


		}
	}

	int pid = fork ();
	if (pid < 0){
		EXITONERROR ("Could not create a child process for running the server");
	}
	if (!pid){ // The server runs in the child process
		char* args[] = {"./server", nullptr};
		char* argsnew[] = {"./server", "-m", (char*) bcapacitystr.c_str(), nullptr};
		if (bcapacitystr == ""){
			if (execvp(args[0], args) < 0){
				EXITONERROR ("Could not launch the server");
			}
		}
		else{
			if (execvp(argsnew[0], argsnew) < 0){
				EXITONERROR ("Could not launch the server");
			}
		}
	}

	FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);
	if(isNewChan)
	{
		Request nc (NEWCHAN_REQ_TYPE);
		chan.cwrite (&nc, sizeof(nc));
		char chanName [1024];
		chan.cread(chanName, sizeof(chanName));
		

		FIFORequestChannel newchan (chanName, FIFORequestChannel::CLIENT_SIDE);

		ofstream of ("newchannel.txt");
		if (t <= -.1 && e <= -1)
		{
			for (int i = 0; i < 1000; i++)
			{
				DataRequest d (p, i*0.004, 1);
				
				newchan.cwrite (&d, sizeof (DataRequest)); // question
				double reply;
				newchan.cread (&reply, sizeof(double)); //answer

				DataRequest dn (p, i*0.004, 2);
				newchan.cwrite (&dn, sizeof (DataRequest)); // question
				double reply2;
				newchan.cread (&reply2, sizeof (double)); //answer

				//cout << i << "," << reply << "," << reply2 << endl;

				of << "For person " << p << ", at time " << i*0.004 << ", the value of" << 1 << " is " << reply << ", and " << 2 << " is: " << reply2 << endl;

		
				// char buf [MAX_MESSAGE];
				// DataRequest dn(p, i, 1);
				// chan.cwrite (&dn, sizeof (DataRequest)); // question
				// double reply;
				// int nbytes = chan.cread (&reply, sizeof(double)); //answer

				// DataRequest y (p, i, 2);
				// chan.cwrite (&y, sizeof (DataRequest)); // question
				// double replyTwo;
				// int nbytesTwo = chan.cread (&replyTwo, sizeof(double)); //answer

				// cout << i << "," << reply << "," << replyTwo << "\n"; 

			}

			auto nc_et_dp = high_resolution_clock::now();

			auto nc_d_dp = duration_cast<microseconds>(nc_et_dp-nc_st_dp);
			cout << "Time taken for new channel data points is: " << nc_d_dp.count() << " microseconds" << endl;

		}
		else {
			DataRequest d (p, t, e);
			newchan.cwrite (&d, sizeof (DataRequest)); // question
			double reply;
			newchan.cread (&reply, sizeof(double)); //answer
			// if (isValidResponse(&reply)){
			of << "For person " << p <<", at time " << t << ", the value of ecg "<< e <<" is " << reply << endl;
			// }
		}
		of.close();

		auto nc_st_ft = high_resolution_clock::now();

		if (filename != "")
		{
			/* this section shows how to get the length of a file
			you have to obtain the entire file over multiple requests 
			(i.e., due to buffer space limitation) and assemble it
			such that it is identical to the original*/
			FileRequest fm (0,0);
			int len = sizeof (FileRequest) + filename.size()+1;
			char buf2 [len];
			memcpy (buf2, &fm, sizeof (FileRequest));
			strcpy (buf2 + sizeof (FileRequest), filename.c_str());
			newchan.cwrite (buf2, len);  
			int64 filelen;
			newchan.cread (&filelen, sizeof(int64));
			if (isValidResponse(&filelen)){
				cout << "File length is: " << filelen << " bytes" << endl;
			}
			
			int64 rem = filelen;
			FileRequest* f = (FileRequest*) buf2;
			ofstream of ("received/" + filename);
			char recvbuf[buffercapacity];
			
			while (rem > 0){
				f->length = min(rem, (int64) buffercapacity);
				newchan.cwrite (buf2, f->length);
				newchan.cread (recvbuf, buffercapacity);
				of.write(recvbuf, f->length);
				rem-= f->length;
				f->offset += f->length;

			}
			of.close();
		}
		auto nc_et_ft = high_resolution_clock::now();
		auto nc_d_ft = duration_cast<microseconds>(nc_et_ft-nc_st_ft);
		cout << "Time taken for new channel file transfer is " << nc_d_ft.count() << " microseconds" << endl;
		


		// Request q (QUIT_REQ_TYPE);
		// chan.cwrite (&q, sizeof(Request));

		// DataRequest d (p, t, e);
		// newchan.cwrite (&d, sizeof (DataRequest)); // question
		// double output;
		// newchan.cread (&output, sizeof(double)); //answer

		// DataRequest d2 (p, t, e);
		// newchan.cwrite (&d2, sizeof(DataRequest)); // question
		// double output2;
		// newchan.cread (&output2, sizeof(double)); // answer

		//wait(0)

		// if (isValidResponse(&output)){
		// }
		Request q (QUIT_REQ_TYPE);
		newchan.cwrite (&q, sizeof (Request));

	}
	else
	{
		auto channel_start_dp = high_resolution_clock::now();

		ofstream of ("channel.txt");
		if (t <= -.1 && e <= -1)
		{
			for (int i = 0; i < 1000; i++)
			{
				DataRequest d (p, i*0.004, 1);
				
				chan.cwrite (&d, sizeof (DataRequest)); // question
				double reply;
				chan.cread (&reply, sizeof(double)); //answer

				DataRequest dn (p, i*0.004, 2);
				chan.cwrite (&dn, sizeof (DataRequest)); // question
				double reply2;
				chan.cread (&reply2, sizeof (double)); //answer

				//cout << i << "," << reply << "," << reply2 << endl;

				of << "For person " << p << ", at time " << i*0.004 << ", the value of" << 1 << " is " << reply << ", and " << 2 << " is: " << reply2 << endl;

				// char buf [MAX_MESSAGE];
				// DataRequest dn(p, i, 1);
				// chan.cwrite (&dn, sizeof (DataRequest)); // question
				// double reply;
				// int nbytes = chan.cread (&reply, sizeof(double)); //answer

				// DataRequest y (p, i, 2);
				// chan.cwrite (&y, sizeof (DataRequest)); // question
				// double replyTwo;
				// int nbytesTwo = chan.cread (&replyTwo, sizeof(double)); //answer

				// cout << i << "," << reply << "," << replyTwo << "\n"; 

			}

			auto channel_end_dp = high_resolution_clock::now();

			auto d_end_dp = duration_cast<microseconds>(channel_end_dp-channel_start_dp);
			cout << "Time taken for original channel data points is: " << d_end_dp.count() << " microseconds" << endl;

		}
		else {
			DataRequest d (p, t, e);
			chan.cwrite (&d, sizeof (DataRequest)); // question
			double reply;
			chan.cread (&reply, sizeof(double)); //answer
			// if (isValidResponse(&reply)){
			cout << "For person " << p <<", at time " << t << ", the value of ecg "<< e <<" is " << reply << endl;
			// }
		}

		auto channel_start_ft = high_resolution_clock::now();
		if (filename != "")
		{
			/* this section shows how to get the length of a file
			you have to obtain the entire file over multiple requests 
			(i.e., due to buffer space limitation) and assemble it
			such that it is identical to the original*/
			FileRequest fm (0,0);
			int len = sizeof (FileRequest) + filename.size()+1;
			char buf2 [len];
			memcpy (buf2, &fm, sizeof (FileRequest));
			strcpy (buf2 + sizeof (FileRequest), filename.c_str());
			chan.cwrite (buf2, len);  
			int64 filelen;
			chan.cread (&filelen, sizeof(int64));
			if (isValidResponse(&filelen)){
				cout << "File length is: " << filelen << " bytes" << endl;
			}
			
			int64 rem = filelen;
			FileRequest* f = (FileRequest*) buf2;
			ofstream of ("received/" + filename);
			char recvbuf[buffercapacity];
			
			while (rem > 0){
				f->length = min(rem, (int64) buffercapacity);
				chan.cwrite (buf2, f->length);
				chan.cread (recvbuf, buffercapacity);
				of.write(recvbuf, f->length);
				rem-= f->length;
				f->offset += f->length;

			}
			of.close();
		}
		
		auto channel_end_ft = high_resolution_clock::now();
		auto d_end_ft = duration_cast<microseconds>(channel_end_ft-channel_start_ft);
		cout << "Time taken for original channel file transfer is: " << d_end_ft.count() << " microseconds" << endl;

	}
	// closing the channel    
	Request q (QUIT_REQ_TYPE);
	chan.cwrite (&q, sizeof (Request));
	// client waiting for the server process, which is the child, to terminate
	wait(0);
	cout << "Client process exited" << endl;

}
