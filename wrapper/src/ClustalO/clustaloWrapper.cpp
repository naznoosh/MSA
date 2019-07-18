#include <fstream>
#include <string>

#include "../progress.h"

using namespace std;


int main(int argc, char* argv[]) {
	int size      = stoi(argv[2]);
	int thread_id = stoi(argv[3]);
	int verbose   = stoi(argv[4]);
	int threads   = stoi(argv[5]);
	ifstream in(argv[1]);
	if (!in.is_open()) return 0;
	string root = argv[0];
#ifndef _MSC_VER
	root.erase(root.find_last_of('/') + 1);
#else
	root.erase(root.find_last_of('\\') + 1);
#endif
//	printf("%s \n", root.c_str());
	Progress prog(size);

	string data_file_name;
	string res_file_name;

	for (int i = 0; i < size; i++)
	{
		if (thread_id == 0 && !verbose) prog.progress(i);

		getline(in, data_file_name);
		getline(in, res_file_name);

		string cmnd;
		cmnd.append(root)
			.append("clustalo --infile=")
			.append(data_file_name).append(" --outfile=")
			.append(res_file_name).append(" --output-order=input-order --force --threads=")
			.append(to_string(threads));
		if (verbose) cmnd.append(" --verbose");
#ifndef _MSC_VER
		else cmnd.append(" 2> /dev/null");
#else
		else cmnd.append(" 2> NUL");
#endif
//		printf("%s \n", cmnd.c_str());
		system(cmnd.c_str());
	}
	if (thread_id == 0 && !verbose) prog.finish();
	
	return 1;
}
