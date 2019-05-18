#ifndef SCENARIO_H
#define SCENARIO_H

//#include "script.h"
#include <string>
#include "dictionary.h"
#include <vector>
#include <set>
#include <cstdlib>
#include <ctime>
#include <Windows.h>
#include <gdiplus.h>
#include <iostream>
#include <cmath>
#include <fstream>	
#include <sstream>
#include <natives.h>
#include <stdexcept>

//#include "keyboard.h"

#pragma comment (lib,"Gdiplus.lib")

//constexpr int SCREEN_WIDTH = 1920;
//constexpr int SCREEN_HEIGHT = 1080;
//constexpr float TIME_FACTOR = 12.0;
// set FPS to one in order to have signifikant changings during recorded frames
//constexpr int FPS = 30;

constexpr int max_number_of_peds = 1024;					// size of the pedestrians array
constexpr int number_of_joints = 21;							// size of the joint_ID subset
//constexpr int max_wpeds = 512;

typedef struct wPed {
	Ped ped;
	bool goingTo = true;
	Vector3 from, to;
	int stopTime;
	int timeFix = -1;
	float speed;
} wPed;

using ParameterString = std::string;

template<typename ValueType>
class Parameters {
public:
	Parameters(const std::string& config_file) :config_file_(config_file) {};
	Parameters() = default;

	std::map<std::string, ValueType> getMap() const {
		return this->parameters_;
	};

	void registerParam(const std::string& key) {

		std::ifstream c_file(this->config_file_);
		if (c_file.is_open()) {
			std::string line;
			while (std::getline(c_file, line))
			{
				// remove spaces from actual line
				line.erase(std::remove_if(line.begin(), line.end(), isspace),
					line.end());
				// parse comment or empty line
				if (line[0] == '#' || line.empty()) {
					continue;
				}
				const auto del_pos = line.find(":");
				const auto act_key = line.substr(0, del_pos);
				if (act_key.compare(key) == 0) {
					// convert key to templated type
					parameters_[key]=convertKey<ValueType>(line.substr(del_pos+1));
					return;
				}
			}
		}
		else {
			throw std::invalid_argument("The speficied Parameterfile " + config_file_ + " does not exist!");
		}

		// This line should never be reached
		throw std::invalid_argument("The Parameter " + key + " does not exist! Check Parameterfile!");
	};

	ValueType getParam(const std::string& key) {
		if (this->parameters_.find(key) == this->parameters_.end()) {
			throw std::invalid_argument("Key " + key + "is not registered! Check its name and value type!");
		}
		return parameters_[key];
	};

private:
	std::map<std::string, ValueType> parameters_;
	const std::string config_file_;

	template<typename ValueType> 
	ValueType convertKey(const std::string& key) const {
		std::stringstream converter(key);
		ValueType value;
		converter >> value;
		return value;
	};

	// explicit specialization is needed for strings
	template<>
	std::string convertKey<std::string>(const std::string& key) const {
		const std::string value = key;
		return value;
	}

	// needed for bools
	template<>
	bool convertKey<bool>(const std::string& key) const {
		std::stringstream converter(key);
		bool value;
		converter >> std::boolalpha >> value;
		return value;
	}



	
};


class DatasetAnnotator
{
public:
	DatasetAnnotator(std::string config_file, int _is_night);
	int update();
	void drawText(const std::string& text) const;
	void updateStatusText() const;
	std::string getOutputPath() const { return this->current_output_path; }
	int getMaxFrames() const { return this->MAX_SAMPLES; }
	void loadScenario();
	void resetStates();
	CLSID getCLSID() const { return this->bmpClsid; }
	CLSID getCLSIDPNG() const { return this->pngClsid; }
	int getWindowWidth() const{ return this->windowWidth; }
	int getWindowHeight() const{ return this->windowHeight; }
	bool recordAllSeqs() const { return this->RECORD_ALL_SEQS; }
	~DatasetAnnotator();

private:
	Parameters<int> int_params_;
	Parameters<float> float_params_;
	Parameters<std::string > string_params_;
	Parameters<bool> bool_params_;
	std::string output_path;
	std::string current_output_path;
	int default_weather_;
	std::set<std::string> scenario_names_, already_recorded_scenarios_;
	int sequence_index;
	Player player;
	Ped playerPed;
	std::string line;								// string use the safe the fram data-line
	std::string log;
	Cam camera;										// camera
	Vector3 cam_coords;								// coordinates of the camera
	Vector3 cam_rot;
	Vector3 wanderer;
	Ped entity_cam;
	Vector3 ped_spawn_pos;
	bool SHOW_JOINT_RECT;							// bool used to switch the rectangle drawing around the joint
	Ped ped_spawned[max_number_of_peds];
	int n_peds;
	std::vector<const char*> bad_scenarios; // 36
	//int ped_with_cam;
	std::string file_scenarios_path;
	
	std::vector<Ped> overall_peds = {};
	int moving;
	Vector3 A, B, C;
	float offset_x = 0.0f;
	float offset_y = 0.0f;

	int SCREEN_HEIGHT, SCREEN_WIDTH, FPS, MAX_NR_WPEDS,MAX_SAMPLES;
	float TIME_FACTOR;
	bool DEBUG_MODE, RECORD_ALL_SEQS;

	int windowWidth;
	int windowHeight;
	int secondsBeforeSaveImages;
	int captureFreq;
	int joint_int_codes[number_of_joints];
	int fov;
	int max_waiting_time = 0;
	int is_night;
	Object seq_count_ = 0;
	int nsample = 0;

	int n_peds_left = max_number_of_peds;

	HWND hWnd;// hWnd1, hWnd2, hChild, hChild1, hChild2;
	HDC hWindowDC;
	HDC hCaptureDC;
	HBITMAP hCaptureBitmap;

	float recordingPeriod;
	std::clock_t lastRecordingTime;
	
	std::ofstream coords_file;								// file used to save joints coordinates data
	std::ofstream log_file;									// logging while updating

	CLSID bmpClsid, pngClsid;
	ULONG_PTR gdiplusToken;

	void registerParams();
	void get_2D_from_3D(Vector3 v, float *x, float *y);														// function used to capture frames internally, then private
	void setCameraFixed(Vector3 coords, Vector3 rot, float cam_z, int fov);
	//void setCameraMoving(Vector3 A, Vector3 B, Vector3 C, int fov);							// function used to set the camera stuff										// function used to spawn pedestrians at the beginning of the scenario
	Vector3 teleportPlayer(Vector3 pos);													// function used to teleport the player to a random position returned by the function
	void spawn_peds_flow(Vector3 pos, Vector3 goFrom, Vector3 goTo, int npeds, int ngroup, int task_time, int type, int radius, 
		int min_lenght, int time_between_walks, int spawning_radius, float speed);
	void spawn_peds(Vector3 pos, Vector3 goFrom, Vector3 goTo, int npeds, int ngroup, int currentBehaviour,
		int task_time, int type, int radius, int min_lenght, int time_between_walks, int spawning_radius, float speed);
	int myreadLine(FILE *f, Vector3 *pos, int *nPeds, int *ngroup, int *currentBehaviour, float *speed, Vector3 *goFrom, Vector3 *goTo, int *task_time,
		int *type, int *radius, int *min_lenght, int *time_between_walks, int *spawning_radius);
	void readInScenarios();
	void addPed(const Ped ped);
	Cam lockCam(Vector3 pos, Vector3 rot);
	Object createNewSeq();
	void save_frame();

	// parameter names
	const ParameterString output_file_param_name_ = "output_file";
	const ParameterString scenario_file_param_name_ = "scenario_file";
	const ParameterString max_samples_param_name_ = "max_nr_samples";
	const ParameterString default_weather_param_name_ = "default_weather_name";
	const ParameterString screen_width_param_name_ = "screen_width";
	const ParameterString screen_height_param_name_ = "screen_height";
	const ParameterString timefactor_param_name_ = "time_factor";
	const ParameterString fps_param_name_ = "frames_per_seq";
	const ParameterString max_wpeds_param_name_ = "max_nr_walking_peds";
	const ParameterString debug_param_name_ = "debug_mode";
	const ParameterString all_seq_param_name_ = "record_all_seqs";
};

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

#endif // !SCENARIO_H
