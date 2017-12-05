#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <queue>
#include <stack>
#include <unordered_map>
#include <fstream>
using namespace std;

//Global parameters
int total_number_of_page_frames;
int maximum_segment_length;
int page_size;
int number_of_page_frames_per_process;
int x;
int min1;
int max1;
int num_of_processes;

vector<int> mainPID;
vector<int> addr;


struct processID
{
	int pid;
	int counter = 0;

	void reset() {
		counter = 0;
	}
};
processID *pid1;
vector<int> frames_on_disk;
vector<string> HEXaddr;


void readFile(string filename) {
	ifstream infile(filename);
	string str;

	if (!infile.is_open()) {
		cout << "ERROR OPENING FILE " << filename << " does not exist" << endl;
		exit(1);
	}
	//read in all global parameters
	else {
		getline(infile, str);
		total_number_of_page_frames = stoi(str);
		getline(infile, str);
		maximum_segment_length = stoi(str);
		getline(infile, str);
		page_size = stoi(str);
		getline(infile, str);
		number_of_page_frames_per_process = stoi(str);
		getline(infile, str);
		x = stoi(str);
		getline(infile, str);
		min1 = stoi(str);
		getline(infile, str);
		max1 = stoi(str);
		getline(infile, str);
		num_of_processes = stoi(str);

		pid1 = new processID[num_of_processes];

		for (int i = 0; i < num_of_processes; i++) {
			getline(infile, str);
			stringstream stream(str);
			string line;

			//cache processes running
			stream >> line;
			pid1[i].pid = stoi(line);

			//cache frames on disk per process
			stream >> line;
			frames_on_disk.push_back(stoi(line));
		}

		//sequentially reads in pages
		while (getline(infile, str)) {
			stringstream stream(str);
			string line;

			//cache processes running
			stream >> line;
			mainPID.push_back(stoi(line));

			//cache pages 
			stream >> line;
			HEXaddr.push_back(line);

			//Convert address string to an int: hex -> int
			char *newStr = new char[line.length() + 1];
			strcpy(newStr, line.c_str());
			char *newLine;
			int addrNum = (int)strtol(newStr, &newLine, 0);
			addr.push_back(addrNum);
		}
	}
	infile.close();
}



//return number of pagefaults for FIFO
int FIFO(vector<int> pages, int frames) {
	unordered_set<int> set;
	queue<int> indexes;
	

	int page_faults = 0;
	for (int i = 0; i<pages.size(); i++) {
		//allocate initial frames
		if (set.size() < frames) {
			if (set.find(pages[i]) == set.end()) {
				set.insert(pages[i]);
				page_faults++;
				for (int k = 0; k < num_of_processes; k++) {
					if (mainPID[i] == pid1[k].pid)
						pid1[k].counter++;
				}
				indexes.push(pages[i]);
			}
		}
		//replace page frames
		else {
			if (set.find(pages[i]) == set.end()) {

				int val = indexes.front();
				indexes.pop();
				set.erase(val);
				set.insert(pages[i]);
				indexes.push(pages[i]);
				for (int k = 0; k < num_of_processes; k++) {
					if (mainPID[i] == pid1[k].pid)
						pid1[k].counter++;
				}
				page_faults++;
			}
		}
	}

	return page_faults;
}


//return number of pagefaults for FIFO
int LIFO(vector<int> pages, int frames) {
	unordered_set<int> set;
	stack<int> indexes;


	int page_faults = 0;
	for (int i = 0; i<pages.size(); i++) {
		//allocate initial frames
		if (set.size() < frames) {
			if (set.find(pages[i]) == set.end()) {
				set.insert(pages[i]);
				page_faults++;
				for (int k = 0; k < num_of_processes; k++) {
					if (mainPID[i] == pid1[k].pid)
						pid1[k].counter++;
				}
				indexes.push(pages[i]);
			}
		}
		//replace pages
		else {
			if (set.find(pages[i]) == set.end()) {

				int val = indexes.top();
				indexes.pop();
				set.erase(val);
				set.insert(pages[i]);
				indexes.push(pages[i]);
				for (int k = 0; k < num_of_processes; k++) {
					if (mainPID[i] == pid1[k].pid)
						pid1[k].counter++;
				}
				page_faults++;
			}
		}
	}

	return page_faults;
}

//return number of pagefaults for LRU-X
int LRU_X(vector<int> pages, int frames, int x)
{
	unordered_set<int> set;

	unordered_map<int, int> indexes;

	int page_faults = 0;
	for (int i = 0; i<pages.size(); i++){
		//allocate initial frames
		if (set.size() < frames){
			if (set.find(pages[i]) == set.end()){
				set.insert(pages[i]);
				for (int k = 0; k < num_of_processes; k++) {
					if (mainPID[i] == pid1[k].pid)
						pid1[k].counter++;
				}
				page_faults++;
			}

			indexes[pages[i]] = i;
		}
		//page replacement
		else{
			if (set.find(pages[i]) == set.end()){
				int lru = INT_MAX, val;
				for (auto i = set.begin(); i != set.end(); i++){
					if (indexes[*i] < lru){
						lru = indexes[*i];
						val = *i;
					}
				}
				set.erase(val);
				set.insert(pages[i]);
				for (int k = 0; k < num_of_processes; k++) {
					if (mainPID[i] == pid1[k].pid)
						pid1[k].counter++;
				}
				
				page_faults++;
			}

			indexes[pages[i]] = i;
		}
	}

	return page_faults;
}



//searches for frames in addr buffer
bool search(int key, vector<int>& fr)
{
	for (int i = 0; i < fr.size(); i++)
		if (fr[i] == key)
			return true;
	return false;
}

//looks ahead X frames
int lookAhead(vector<int> pg, vector<int>& fr, int index, int x)
{
	int res = -1, farthest = index;
	for (int i = 0; i < fr.size(); i++) {
		int j;
		//only goes up to x referneces. Highly inefficient, why not just use regular OPT algo?? 
		//regular OPT causes much fewer page faults...
		//changing the j<x to j<addr.size() yield 131% fewer pagefaults
		for (j = index; j < addr.size(); j++) {
			if (fr[i] == pg[j]) {
				if (j > farthest) {
					farthest = j;
					res = i;
				}
				break;
			}
		}

		if (j == addr.size())
			return i;
	}

	return (res == -1) ? 0 : res;
}

//return number of pagefaults for OPT-X
int OPT(vector<int> pg, int fn, int x)
{
	vector<int> fr;

	int hit = 0;
	for (int i = 0; i < addr.size(); i++) {
		//if search yeilds true page fault doesnt occur
		if (search(pg[i], fr)) {
			hit++;
			continue;
		}
		//pushes back frame
		if (fr.size() < fn) {
			fr.push_back(pg[i]);
			for (int k = 0; k < num_of_processes; k++) {
				if (mainPID[i] == pid1[k].pid)
					pid1[k].counter++;
			}
		}
		else {
			int j = lookAhead(pg, fr, i + 1, x);
			for (int k = 0; k < num_of_processes; k++) {
				if (mainPID[i] == pid1[k].pid)
					pid1[k].counter++;
			}
			fr[j] = pg[i];
		}
	}
	
	return addr.size() - hit;
}

//calculates logest distance betweeen pages
int distance(int size, int i, int j) {
	int a = (i - j) % size;
	int b = (j - i) % size;
	return max(a, b);
}

//return number of pagefaults for LDF
int LDF(vector<int> pages, int frames)
{
	//no dulplicates in set
	vector<int> vec = pages;
	sort(vec.begin(), vec.end());
	vec.erase(unique(vec.begin(), vec.end()), vec.end());
	
	unordered_set<int> set;

	unordered_map<int, int> indexes;

	int page_faults = 0;
	for (int i = 0; i<pages.size(); i++) {
		//allocate initial frames
		if (set.size() < frames) {
			if (set.find(pages[i]) == set.end()) {
				set.insert(pages[i]);
				for (int k = 0; k < num_of_processes; k++) {
					if (mainPID[i] == pid1[k].pid)
						pid1[k].counter++;
				}
				page_faults++;
			}

			indexes[pages[i]] = i;
		}
		//page replacement
		else {
			if (set.find(pages[i]) == set.end()) {
				int ldf = INT_MAX, val;
				int a = 0;
				for (auto i = set.begin(); i != set.end(); i++) {
					//calculates distance and repllacese pages
					if (indexes[*i] > distance(vec.size(), a, ldf)) {
						ldf = indexes[*i];
						val = *i;
						
					}
					a++;
				}
				set.erase(val);
				set.insert(pages[i]);
				for (int k = 0; k < num_of_processes; k++) {
					if (mainPID[i] == pid1[k].pid)
						pid1[k].counter++;
				}

				page_faults++;
			}

			indexes[pages[i]] = i;
		}
	}

	return page_faults;
}


int WS(vector<int> pages, int frames, int delta) {
	unordered_set<int> set;

	queue<int> indexes;

	int page_faults = 0;
	for (int i = 0; i<pages.size(); i++) {
		//allocate initial frames
		for (int j = 0; j < delta; j++) {
			if (set.size() < frames) {
				if (set.find(pages[i]) == set.end()) {
					set.insert(pages[i]);
					for (int k = 0; k < num_of_processes; k++) {
						if (mainPID[i] == pid1[k].pid)
							pid1[k].counter+=3;
					}
					page_faults += 3;
				}

				indexes.push(pages[i]);
			}
			//page replacement
			else {
				if (set.find(pages[i]) == set.end()) {

					int val = indexes.front();
					indexes.pop();
					set.erase(val);
					set.insert(pages[i]);
					indexes.push(pages[i]);
					for (int k = 0; k < num_of_processes; k++) {
						if (mainPID[i] == pid1[k].pid)
							pid1[k].counter++;
					}
					page_faults++;
				}
			}
		}
	}

	return page_faults;
}









int main(int argc, char *argv[]) {
	readFile(argv[1]);


	int a = FIFO(addr, number_of_page_frames_per_process);
	for (int i = 0; i < num_of_processes; i++) {
		cout <<pid1[i].pid<<": "<< pid1[i].counter << endl;
	}
	cout << "Fifo total page faults: " << a << endl;
	for (int i = 0; i < num_of_processes; i++) {
		pid1[i].reset();
	}
	

	cout << "-----------------------------------------------------------" << endl;
	int d = LIFO(addr, number_of_page_frames_per_process);
	for (int i = 0; i < num_of_processes; i++) {
		cout << pid1[i].pid << ": " << pid1[i].counter << endl;
	}
	cout << "Lifo total page faults: " << d << endl;
	for (int i = 0; i < num_of_processes; i++) {
		pid1[i].reset();
	}

	cout << "-----------------------------------------------------------" << endl;

	int c = LRU_X(addr, number_of_page_frames_per_process, x);
	for (int i = 0; i < num_of_processes; i++) {
		cout << pid1[i].pid << ": " << pid1[i].counter << endl;
	}
	cout << "LRU-X total page faults: " << c << endl;
	for (int i = 0; i < num_of_processes; i++) {
	pid1[i].reset();
	}
	cout << "-----------------------------------------------------------" << endl;

	int b =  OPT(addr, number_of_page_frames_per_process, x);
	for (int i = 0; i < num_of_processes; i++) {
		cout << pid1[i].pid << ": " << pid1[i].counter << endl;
	}
	cout << "OPT-X total page faults: " << b << endl;
	for (int i = 0; i < num_of_processes; i++) {
		pid1[i].reset();
	}

	cout << "-----------------------------------------------------------" << endl;

	int u = LDF(addr, number_of_page_frames_per_process);
	for (int i = 0; i < num_of_processes; i++) {
		cout << pid1[i].pid << ": " << pid1[i].counter << endl;
	}
	cout << "LDF total page faults: " << u << endl;
	for (int i = 0; i < num_of_processes; i++) {
		pid1[i].reset();
	}

	cout << "-----------------------------------------------------------" << endl;

	int t = WS(addr, 4, number_of_page_frames_per_process);
	int n = floor(addr.size() / total_number_of_page_frames);
	
	
	for (int i = 0; i < num_of_processes; i++) {
		cout << pid1[i].pid << ": " << pid1[i].counter << endl;
	}
	cout << "WS Min: " << min1 << endl;
	cout << "WS Max: " << max1 << endl;
	cout << "WS total page faults: " << t << endl;
	for (int i = 0; i < num_of_processes; i++) {
		pid1[i].reset();
	}
	
	return 0;
}
