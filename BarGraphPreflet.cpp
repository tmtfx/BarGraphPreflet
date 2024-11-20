/*
 * Copyright 2024, Fabio Tomat <f.t.public@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Application.h>
#include <Looper.h>
#include <Window.h>
#include <View.h>
#include <TextControl.h>
#include <Button.h>
#include <Slider.h>
#include <Menu.h>
#include <MenuField.h>
#include <Message.h>
#include <MenuItem.h>
#include <String.h>
#include <StringList.h>
#include <StringView.h>
#include <InterfaceKit.h>
#include <Alert.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstdlib>

const char* kApplicationSignature = "application/x-vnd.BarGraph-Preflet";
const std::vector<std::string> labeloptions = {"1:", "2:", "3:", "4:", "5:", "6:", "7:", "8:", "M:", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8"};
bool daemonStatus = false;
static const uint32 DAEMON_STATUS = 'DSTS';
static const uint32 UPDATE_NUM_BARS = 'upnb';

struct Config {
        std::string serialPort;
        bool showLabels;
        int numBars;
		int brightness;
        std::vector<std::string> labels;
    };
class BMySlider : public BSlider {
public:
    BMySlider(BRect frame, const char* name, const char* label,
                 BMessage* message, int32 minValue, int32 maxValue, 
                 thumb_style thumbType = B_BLOCK_THUMB,
                 uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP)
        : BSlider(frame, name, label, message, minValue, maxValue, thumbType, resizingMode)
    {}

    void MouseUp(BPoint where) override {
        BSlider::MouseUp(where);
        ExecuteOnMouseUp();
    }

private:
    void ExecuteOnMouseUp() {
        be_app->WindowAt(0)->PostMessage(UPDATE_NUM_BARS);
    }
};

class BarGraphPrefletWindow : public BWindow {
public:
    BarGraphPrefletWindow(Config& config)
        : BWindow(BRect(100, 100, 400, 840), "Bar Graph Settings", B_TITLED_WINDOW, B_NOT_RESIZABLE),
          fConfig(config)
    {
        mainView = new BView(Bounds(), "MainView", B_FOLLOW_ALL, B_WILL_DRAW);
        mainView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

        fSerialPortControl = new BTextControl(BRect(10, 10, 290, 30), "SerialPort", "Serial Port:", fConfig.serialPort.c_str(), new BMessage(SERIAL_PATH));
		
		fLabels.resize(fConfig.numBars, "");

        fShowLabelsButton = new BButton(BRect(10, 50, 200, 75), "ShowLabels", "Toggle Labels", new BMessage(TOGGLE_LABELS));
		fShowLabelsButton->SetEnabled(false);
		fResetBSButton = new BButton(BRect(200, 50, 290, 75), "resetBarSettings", "Reset bars", new BMessage(RESET_BARS));
        fBacklightSlider = new BSlider(BRect(10, 100, 290, 125), "Backlight", "Backlight", new BMessage(CHANGE_BACKLIGHT), 0, 100);
        fBacklightSlider->SetValue(fConfig.brightness);
		fBacklightSlider->SetEnabled(false);
		fnumBarsSlider = new BMySlider(BRect(10, 150, 290, 175),"numBarsSlider", "Number of Bars", new BMessage(UPDATE_NUM_BARS), 1, 8);
		fnumBarsSlider->SetKeyIncrementValue(1);
		fnumBarsSlider->SetValue(fConfig.numBars);
		fnumBarsSlider->SetEnabled(false);
        fBarSettings = new BView(BRect(10, 240, 290, 485), "BarSettings", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
		fConfigLabelsButton = new BButton(BRect(10, 490, 290, 530), "configLabels", "Send labels configuration", new BMessage(CONFIGURE_LABELS));
		fConfigLabelsButton->SetEnabled(false);
		fKillDaemonButton = new BButton(BRect(10, 532, 290, 572), "killDaemon", "Ask Daemon to Quit", new BMessage(REMOTE_QUIT_REQUEST));
		fKillDaemonButton->SetEnabled(false);
		fshutdownText = new BTextControl(BRect(10, 580, 290, 625), "shutdownText", "Shutdown Text:", "", new BMessage(SHTDWN_TXT_CHG));
		const char* lblstatus = "Daemon status: ";
		flabelStatus = new BStringView(BRect(10,630,mainView->StringWidth(lblstatus)+10,655),"lblstatus",lblstatus);
		const char* daemonstatus = "Not running";
		fdaemonStatus = new BStringView(BRect(290-mainView->StringWidth(daemonstatus),630,290,655),"daemonStatus",daemonstatus);
		fdaemonStatus->SetHighColor(255,0,0,255);
		fdaemonStatus->SetAlignment(B_ALIGN_RIGHT);
		fLaunchDaemonButton = new BButton(BRect(10, 680, 290, 730), "launchDaemon", "Launch BarGraphDaemon", new BMessage(LAUNCH_DAEMON));
		fgraphicCheckBox = new BCheckBox(BRect(10, 200, 290, 230),"graphicEnabler","Fullscreen graphic?",new BMessage(bGRAPHIC));
		fgraphicCheckBox->SetEnabled(false);
		for (int i = 0; i < fConfig.numBars; i++) {
            BMenu* menu = new BMenu("Select Label");
            for (const auto& option : labeloptions) {
				BMessage* msg = new BMessage(SET_LABEL);
				msg->AddInt32("index",i);
				msg->AddString("label",option.c_str());
                BMenuItem* item = new BMenuItem(option.c_str(), msg);
                menu->AddItem(item);
            }
			menu->SetLabelFromMarked(true);
			std::string lbl = "Bar label " +std::to_string(i+1);
            BMenuField* menuField = new BMenuField(BRect(10, 30 * i, 300, 30 * (i + 1)), nullptr, lbl.c_str(), menu);
            fBarSettings->AddChild(menuField);
        }
		
        mainView->AddChild(fSerialPortControl);
        mainView->AddChild(fShowLabelsButton);
		mainView->AddChild(fResetBSButton);
        mainView->AddChild(fBacklightSlider);
		mainView->AddChild(fnumBarsSlider);
        mainView->AddChild(fBarSettings);
		mainView->AddChild(fConfigLabelsButton);
		mainView->AddChild(fKillDaemonButton);
		mainView->AddChild(fshutdownText);
		mainView->AddChild(flabelStatus);
		mainView->AddChild(fdaemonStatus);
		mainView->AddChild(fLaunchDaemonButton);
		mainView->AddChild(fgraphicCheckBox);
        AddChild(mainView);
    }
	
	void TransmitToDaemon(BMessage* message){
		BMessenger messenger("application/x-vnd.BarGraphDaemon");
		messenger.SendMessage(message);
	}
	
	void UpdateLabelsArray(int newNumBars) {
        fLabels.clear();
        fLabels.resize(newNumBars, "");
    }	
	
	void ResetBarSettings(){
	
		for (int i = fBarSettings->CountChildren() - 1; i >= 0; --i) {
			BMenuField* menuField = dynamic_cast<BMenuField*>(fBarSettings->ChildAt(i));
			if (menuField) {
				fBarSettings->RemoveChild(menuField);
			}
		}
		int newValue = fnumBarsSlider->Value();
		
		for (int i = 0; i < newValue; i++) {
			BMenu* menu = new BMenu("Select Label");
			for (const auto& option : labeloptions) {
				BMessage* msg = new BMessage(SET_LABEL);
				msg->AddInt32("index",i);
				msg->AddString("label",option.c_str());
				BMenuItem* item = new BMenuItem(option.c_str(), msg);
				menu->AddItem(item);
			}
			menu->SetLabelFromMarked(true);
			std::string lbl = "Bar label "+std::to_string(i+1);
			BMenuField* menuField = new BMenuField(BRect(10, 30 * i, 300, 30 * (i + 1)), nullptr, lbl.c_str(), menu);
			fBarSettings->AddChild(menuField);
		}
	}
	
	virtual void MessageReceived(BMessage* message) override {
		//message->PrintToStream();
		switch (message->what) {
			case UPDATE_NUM_BARS:
				{
					int32 newNumBars = fnumBarsSlider->Value();//message->FindInt32("value");
					UpdateLabelsArray(newNumBars);
					ResetBarSettings();
					/*if (newNumBars != fConfig.numBars) {
						fConfig.numBars = newNumBars;
						//printf("ora fConfig.numBars è: %d\n", fConfig.numBars);
						
					}*/
					printf("New numBars: %s\n",std::to_string(newNumBars).c_str());
					BRect bsrect = fBarSettings->Bounds();
					float h = bsrect.Height();
					if (newNumBars == 1) {
						fgraphicCheckBox->SetEnabled(true);
					} else {
						fgraphicCheckBox->SetEnabled(false);
					}
				}
				break;
			case SET_LABEL:
				{
					int index = message->FindInt32("index");
					std::string label = message->FindString("label");
					if (index >= 0 && index < fLabels.size()) {
						fLabels[index] = label;
					}
				}
				break;
			case REMOTE_QUIT_REQUEST:
				{
					const char * sht = fshutdownText->Text();
					size_t shtSize = strlen(sht);
					if (shtSize>40) {
						const size_t maxLength = 40;
						char truncatedString[maxLength + 1];
						strncpy(truncatedString, sht, maxLength);
						truncatedString[maxLength] = '\0';
						fshutdownText->SetText(truncatedString);
						sht = truncatedString;
					}
					if (shtSize>0)	message->AddString("text",sht);
					TransmitToDaemon(message);
				}
				break;
			case TOGGLE_LABELS:
				TransmitToDaemon(message);
				break;
			case CHANGE_BACKLIGHT:
				message->AddInt32("bright",fBacklightSlider->Value());
				TransmitToDaemon(message);
				break;
			case RESET_BARS:
				ResetBarSettings();
				break;
			case CONFIGURE_LABELS:
				{
					int newValue = fnumBarsSlider->Value();
					
					bool allLabelsAssigned = true;
					for (int i = 0; i < newValue; i++) {
						if (fLabels[i].empty()) {
							allLabelsAssigned = false;
							break;
						}
					}
					if (allLabelsAssigned) {
						BStringList labelList(newValue);
						for (const std::string& label : fLabels){
							labelList.Add(BString(label.c_str()));
						}
						message->AddInt32("numBars", newValue);
						message->AddStrings("labels",labelList);
						/*for (int i = 0; i < fConfig.numBars; i++) {
							std::string indexStr = std::to_string(i);
							message->AddStrings(indexStr.c_str(), fLabels[i].c_str());
						}*/
						
						TransmitToDaemon(message);
						if (newValue == 1) {
							BMessage* graphicmode = new BMessage(SPECIAL_GRAPH);
							graphicmode->AddBool("graphicMode", bgraph);
							TransmitToDaemon(graphicmode);
						}
					} else {
						BAlert* alert = new BAlert("error","You must assign all labels first!", "OK", nullptr,nullptr,B_WIDTH_AS_USUAL,B_WARNING_ALERT);
						alert->Go();
					}
				}
				break;
			case SERIAL_PATH:
				{
					message->AddString("path",fSerialPortControl->Text());
					TransmitToDaemon(message);
				}
				break;
			case DAEMON_STATUS:
				{
					bool status = message->FindBool("status");
					if (prevStat != status) {
						if (status) {
							fdaemonStatus->SetText("Running!");
							fnumBarsSlider->SetEnabled(true);
							fBacklightSlider->SetEnabled(true);
							fKillDaemonButton->SetEnabled(true);
							fConfigLabelsButton->SetEnabled(true);
							fShowLabelsButton->SetEnabled(true);
							fLaunchDaemonButton->SetEnabled(false);
							fdaemonStatus->SetHighColor(0,200,0,255);
						} else {
							fdaemonStatus->SetText("Not running");
							fnumBarsSlider->SetEnabled(false);
							fBacklightSlider->SetEnabled(false);
							fKillDaemonButton->SetEnabled(false);
							fConfigLabelsButton->SetEnabled(false);
							fShowLabelsButton->SetEnabled(false);
							fLaunchDaemonButton->SetEnabled(true);
							fdaemonStatus->SetHighColor(255,0,0,255);
						}
						prevStat=status;
					}
				}
				break;
			case LAUNCH_DAEMON:
				system("/boot/home/Documents/Progjets/bargraph/bargraph/BarGraphDaemon &");
				break;
			case SHTDWN_TXT_CHG:
				{
					const char * sht = fshutdownText->Text();
					size_t shtSize = strlen(sht);
					if (shtSize>40) {
						const size_t maxLength = 40;
						char truncatedString[maxLength + 1];
						strncpy(truncatedString, sht, maxLength);
						truncatedString[maxLength] = '\0';
						fshutdownText->SetText(truncatedString);
					}
				}
				break;
			case bGRAPHIC:
				{
					bgraph = fgraphicCheckBox->Value();
				}
				break;
			default:
				BWindow::MessageReceived(message);
				break;
		}
	}

	virtual bool QuitRequested() override {
		be_app->PostMessage(B_QUIT_REQUESTED);
		return true;
	}
	
private:
    Config& fConfig;
	BView* mainView;
    BTextControl* fSerialPortControl;
    BButton* fShowLabelsButton;
    BButton* fConfigLabelsButton;
	BButton* fResetBSButton;
	BButton* fKillDaemonButton;
    BSlider* fBacklightSlider;
    BView* fBarSettings;
	BMySlider* fnumBarsSlider;
	BTextControl* fshutdownText;
	BStringView* flabelStatus;
	BStringView* fdaemonStatus;
	std::vector<std::string> fLabels;
	BButton* fLaunchDaemonButton;
	BCheckBox* fgraphicCheckBox;
	
	bool bgraph=false;
	bool prevStat=false;
    static const uint32 TOGGLE_LABELS = 'SLAB';
    static const uint32 CHANGE_BACKLIGHT = 'SETB';
    static const uint32 SET_LABEL = 'stlb';
	static const uint32 CONFIGURE_LABELS = 'SCFG';
	static const uint32 REMOTE_QUIT_REQUEST = '_RQR';
	static const uint32 RESET_BARS = 'RSBR';
	static const uint32 SERIAL_PATH = 'SPTH';
	static const uint32 DAEMON_PING = 'PING';
	static const uint32 LAUNCH_DAEMON = 'LNCD';
	static const uint32 SHTDWN_TXT_CHG = 'SDTC';
	static const uint32 bGRAPHIC = 'TOGG';
	static const uint32 SPECIAL_GRAPH = 'GRPH';
};


class BarGraphPreflet : public BApplication {
public:
	BarGraphPreflet()
        : BApplication(kApplicationSignature) {
			SetPulseRate(1500000);
			Config config;
			config = loadConfig();
			fwindow = new BarGraphPrefletWindow(config);
			fwindow->Show();
	}
	virtual void Pulse() override {
			if (!daemonStatus) {
				BMessage* msg = new BMessage(DAEMON_STATUS);
				msg->AddBool("status",false);
				be_app->WindowAt(0)->PostMessage(msg);
			}
			daemonStatus=false;
			BMessage* message = new BMessage(DAEMON_PING);
			BMessenger messenger("application/x-vnd.BarGraphDaemon");
			messenger.SendMessage(message);
	}
	virtual void MessageReceived(BMessage* message) override {
		//message->PrintToStream();
		switch (message->what) {
			case DAEMON_PING:
				{
					daemonStatus=true;
					BMessage* msg = new BMessage(DAEMON_STATUS);
					msg->AddBool("status",true);
					fwindow->PostMessage(msg);
				}
				break;
			default:
                BApplication::MessageReceived(message);
                break;
		}
	}
private:
	
	BarGraphPrefletWindow* fwindow;

	Config loadConfig() {
        Config config;
        std::ifstream configFile("/boot/system/settings/bargraph.conf");

        if (configFile.is_open()) {
            std::getline(configFile, config.serialPort);
			//fprintf(stdout, "serial port: %s\n", config.serialPort.c_str());
            configFile >> config.showLabels >> config.numBars >> config.brightness;
            std::string label;
            while (configFile >> label) {
                config.labels.push_back(label);
            }			
        } else {
            // Configurazione di default
            config.serialPort = "/dev/ports/usb0";
            config.showLabels = true;
            config.numBars = 8;
			config.brightness = 80;
            config.labels = {"1:", "2:", "3:", "4:","F1", "F2", "F3", "F4"};
            saveConfig(config);
        }

        return config;
    }
	void saveConfig(const Config& config) {
        std::ofstream configFile("/boot/system/settings/bargraph.conf");
        if (configFile.is_open()) {
            configFile << config.serialPort << "\n";
            configFile << config.showLabels << " " << config.numBars << " " << config.brightness << "\n";
            for (const auto& label : config.labels) {
                configFile << label << " ";
            }
        }
    }
	static const uint32 DAEMON_PING = 'PING';
};

int
main()
{
	BarGraphPreflet* app = new BarGraphPreflet();
	app->Run();	
	delete app;
	return 0;
}