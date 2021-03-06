#ifndef SCENARIO_H
#define SCENARIO_H

#include "script.h"
#include <string>
#include "dictionary.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <Windows.h>
#include <gdiplus.h>
#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>
#include "keyboard.h"

#pragma comment (lib,"Gdiplus.lib")

constexpr int MAX_NUMBER_OVERALL_PEDS = 1024;

template<typename ValueType>
class Parameters {
public:
	Parameters(const std::string& config_file) :config_file_(config_file),log_file_(std::string(typeid(ValueType).name()) + "_params_log.txt", std::ios::out){
		// create unique param file name 
		if (!log_file_.is_open()) {
			log_file_.open(std::string(typeid(ValueType).name()) + "_params_log.txt", std::ios::out);
		}	
	};

	Parameters() = default;
	
	~Parameters() {
		log_file_.close();
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
					parameters_[key] = convertKey<ValueType>(line.substr(del_pos + 1));
					const std::string message = "Parameter \"" + key + "\" is registered!\n";
					log_file_.write(message.c_str(), std::strlen(message.c_str()));
					return;
				}
			}
		}
		else {
			const std::string message = "Error: The speficied Parameterfile " + config_file_ + " does not exist!\n";
			log_file_.write(message.c_str(), std::strlen(message.c_str()));
			throw std::invalid_argument("The speficied Parameterfile " + config_file_ + " does not exist!");
		}

		// This line should never be reached
		const std::string message = "Error: The Key " + key + " does not exist!\n";
		log_file_.write(message.c_str(),std::strlen(message.c_str()));
		throw std::invalid_argument("The Parameter " + key + " does not exist! Check Parameterfile!");
	};

	ValueType getParam(const std::string& key) {
		if (this->parameters_.find(key) == this->parameters_.end()) {
			const std::string message = "Error: The Key " + key + " is not registered!\n";
			log_file_.write(message.c_str(),std::strlen(message.c_str()));
			throw std::invalid_argument("Key " + key + "is not registered! Check its name and value type!");
		}
		return parameters_[key];
	};

private:
	std::map<std::string, ValueType> parameters_;
	const std::string config_file_;
	std::ofstream log_file_;

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


};


class ScenarioCreator
{
public:
	ScenarioCreator(const std::string& parameters_file = "param/parameters.txt");
	void update();
	~ScenarioCreator();

private:
	Parameters<std::string> string_params_;
	Parameters<int> int_params_;
	Player player;
	Ped playerPed;
	std::string line;								// string use the safe the fram data-line
	std::string log;
	Cam camera;										// camera
	Vector3 cam_coords;								// coordinates of the camera
	Vector3 cam_rot;
	bool SHOW_JOINT_RECT;							// bool used to switch the rectangle drawing around the joint

	int windowWidth;
	int windowHeight;
	int imageWidth;
	int imageHeight;
	int secondsBeforeSaveImages;
	int captureFreq;
	int joint_int_codes[22];

	std::string files_path_;
	int default_weather_;
	int max_nr_peds_per_spawn;
	int nr_peds_left_;


	HWND hWnd;
	HDC hWindowDC;
	HDC hCaptureDC;
	HBITMAP hCaptureBitmap;

	float recordingPeriod;
	std::clock_t lastRecordingTime;
	int nsample;
	std::ofstream coords_file;									// file used to save joints coordinates data
	std::ofstream ped_spawn_log;								// file used to save joints coordinates data
	std::ofstream log_file;

	CLSID pngClsid;

	void registerParams();
	void cameraCoords();										// function used to show the camera coordinates
	void spawn_peds(Vector3 spawnAreaCenter, int numPed);		// function used to spawn pedestrians at the beginning of the scenario
	void listen_for_keystrokes();								// function used for keyboard input
	void main_menu();											// function used to load the menu
	void resetMenuCommands();									// reset menu commands state
	void weather_menu();										// function used to load the weather menu
	void time_menu();											// function used to load the time menu
	void place_menu();											// function used to load the place menu
	void camera_menu();											// function used to load the camera menu
	void peds_menu();											// function used to load the peds menu
	void tasks_sub_menu();										// to change behaviour settings
	void stopControl();											// to fix player coords
	void walking_peds();										// to make peds walking
	void file_menu();											// function used to load or save files
	void cancelLastLog();

	void draw_text(char *text , float x, float y, float scale);

	bool keyboard_flag;

	const std::string scenario_file_param_name_ = "scenario_file";
	const std::string def_weather_param_name_ = "default_weather_name";
	const std::string max_spawn_peds_param_name_ = "max_nr_peds_per_spawn";
};

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

#endif // !SCENARIO_H
