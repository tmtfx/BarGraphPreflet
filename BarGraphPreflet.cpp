/*
 * Copyright 2024, Fabio Tomat <f.t.public@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

// TODO: Pulse a ping every 3seconds to bargraph daemon to check if it is running
// TODO: if the above is not running enable a Button to launch bargraph daemon

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

struct Config {
        std::string serialPort;
        bool showLabels;
        int numBars;
		int brightness;
        std::vector<std::string> labels;
    };
	
class BarGraphPrefletWindow : public BWindow {
public:
    BarGraphPrefletWindow(Config& config)
        : BWindow(BRect(100, 100, 400, 800), "Bar Graph Settings", B_TITLED_WINDOW, B_NOT_RESIZABLE),
          fConfig(config)
    {
        // Layout
        mainView = new BView(Bounds(), "MainView", B_FOLLOW_ALL, B_WILL_DRAW);
        mainView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

        // Serial Port
        fSerialPortControl = new BTextControl(BRect(10, 10, 290, 30), "SerialPort", "Serial Port:", fConfig.serialPort.c_str(), new BMessage(SERIAL_PATH));
		
		fLabels.resize(fConfig.numBars, "");
		
        // Show Labels
        fShowLabelsButton = new BButton(BRect(10, 50, 200, 75), "ShowLabels", "Toggle Labels", new BMessage(TOGGLE_LABELS));
		fResetBSButton = new BButton(BRect(200, 50, 290, 75), "resetBarSettings", "Reset bars", new BMessage(RESET_BARS));
        // Backlight
        fBacklightSlider = new BSlider(BRect(10, 100, 290, 125), "Backlight", "Backlight", new BMessage(CHANGE_BACKLIGHT), 0, 100);
        fBacklightSlider->SetValue(fConfig.brightness);
		fnumBarsSlider = new BSlider(BRect(10, 150, 290, 175),"numBarsSlider", "Number of Bars", new BMessage(UPDATE_NUM_BARS), 1, 8);
		fnumBarsSlider->SetValue(fConfig.numBars);
		fnumBarsSlider->SetKeyIncrementValue(1);
        // Bar Settings
        fBarSettings = new BView(BRect(10, 200, 290, 445), "BarSettings", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
		fConfigLabelsButton = new BButton(BRect(10, 450, 290, 475), "configLabels", "Send labels configuration", new BMessage(CONFIGURE_LABELS));
		fKillDaemonButton = new BButton(BRect(10, 490, 290, 515), "killDaemon", "Ask Daemon to Quit", new BMessage(REMOTE_QUIT_REQUEST));
		fKillDaemonButton->SetEnabled(false);
		fshutdownText = new BTextControl(BRect(10, 550, 290, 575), "shutdownText", "Shutdown Text:", "", nullptr);
		const char* lblstatus = "Daemon status: ";
		flabelStatus = new BStringView(BRect(10,600,mainView->StringWidth(lblstatus)+10,625),"lblstatus",lblstatus);
		const char* daemonstatus = "Not running";
		fdaemonStatus = new BStringView(BRect(290-mainView->StringWidth(daemonstatus),600,290,625),"daemonStatus",daemonstatus);
		fdaemonStatus->SetHighColor(255,0,0,255);
		fdaemonStatus->SetAlignment(B_ALIGN_RIGHT);
		fLaunchDaemonButton = new BButton(BRect(10, 640, 290, 690), "launchDaemon", "Launch BarGraphDaemon", new BMessage(LAUNCH_DAEMON));
        for (int i = 0; i < fConfig.numBars; i++) {
            BMenu* menu = new BMenu("Select Label");
            //const std::vector<std::string> options = {"1:", "2:", "3:", "4:", "M:", "F1", "F2", "F3", "F4"};
            for (const auto& option : labeloptions) {
				BMessage* msg = new BMessage(SET_LABEL);
				msg->AddInt8("index",i);
				msg->AddString("label",option.c_str());
                BMenuItem* item = new BMenuItem(option.c_str(), msg);
                //item->SetTarget(this);
                menu->AddItem(item);
            }
			menu->SetLabelFromMarked(true);
			std::string lbl = "Bar label " +std::to_string(i+1);
            BMenuField* menuField = new BMenuField(BRect(10, 30 * i, 300, 30 * (i + 1)), nullptr, lbl.c_str(), menu);
            fBarSettings->AddChild(menuField);
        }
		
		fSerialPortControl->SetEnabled(false);
		fBacklightSlider->SetEnabled(false);
		fConfigLabelsButton->SetEnabled(false);
		fShowLabelsButton->SetEnabled(false);
		fResetBSButton->SetEnabled(false);
		fnumBarsSlider->SetEnabled(false);
		fBarSettings->Hide();
		
        // Add everything to main view
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
        AddChild(mainView);
    }
	
	void TransmitToDaemon(BMessage* message){
		BMessenger messenger("application/x-vnd.BarGraphDaemon");
		messenger.SendMessage(message);
	}
	
	void UpdateLabelsArray(int newNumBars) {
        fLabels.clear();  // Pulisce l'array
        fLabels.resize(newNumBars, "");  // Ridimensiona in base al nuovo numero di barre
    }
	
	void ResetBarSettings(){
		for (int i = fBarSettings->CountChildren() - 1; i >= 0; --i) {
			BMenuField* menuField = dynamic_cast<BMenuField*>(fBarSettings->ChildAt(i));
			if (menuField) {
				fBarSettings->RemoveChild(menuField);
			}
		}
		fConfig.numBars = fnumBarsSlider->Value();
		for (int i = 0; i < fConfig.numBars; i++) {
			BMenu* menu = new BMenu("Select Label");
			for (const auto& option : labeloptions) {
				BMessage* msg = new BMessage(SET_LABEL);
				msg->AddInt8("index",i);
				msg->AddString("label",option.c_str());
				BMenuItem* item = new BMenuItem(option.c_str(), msg);
				menu->AddItem(item);
			}
			menu->SetLabelFromMarked(true);
			std::string lbl = "Bar label "+std::to_string(i+1);
			BMenuField* menuField = new BMenuField(BRect(10, 30 * i, 300, 30 * (i + 1)), nullptr, lbl.c_str(), menu);
			fBarSettings->AddChild(menuField);
		}

		// Ridisegna la finestra
		fBarSettings->Invalidate();
		mainView->Invalidate();
	}
	
	virtual void MessageReceived(BMessage* message) override {
		//message->PrintToStream();
		switch (message->what) {
			case UPDATE_NUM_BARS:
				{
					int32 newNumBars = fnumBarsSlider->Value();//message->FindInt32("value");  // Usa FindInt32 invece di FindInt8
					if (newNumBars != fConfig.numBars) {
						fConfig.numBars = newNumBars;
						UpdateLabelsArray(newNumBars);
						ResetBarSettings();
					}
				}
				break;
			case SET_LABEL:
				{
					int index = message->FindInt8("index");
					//fprintf(stdout,"index è %d\n",index);
					std::string label = message->FindString("label");
					//fprintf(stdout,"la label è %s\n",label.c_str());
					if (index >= 0 && index < fLabels.size()) {
						fLabels[index] = label;  // Aggiorna l'etichetta nella posizione corretta
					}
				}
				break;
			case REMOTE_QUIT_REQUEST:
				message->AddString("text",fshutdownText->Text());
				TransmitToDaemon(message);
				break;
			case TOGGLE_LABELS:
				TransmitToDaemon(message);
				break;
			case CHANGE_BACKLIGHT:
				//int val=fBacklightSlider->Value();
				message->AddInt8("bright",fBacklightSlider->Value());
				TransmitToDaemon(message);
				break;
			case RESET_BARS:
				ResetBarSettings();
				break;
			case CONFIGURE_LABELS:
				{
					bool allLabelsAssigned = true;
					for (int i = 0; i < fConfig.numBars; i++) {
						if (fLabels[i].empty()) {
							allLabelsAssigned = false;
							break;
						}
					}
					if (allLabelsAssigned) {
						BStringList labelList(fConfig.numBars);
						for (const std::string& label : fLabels){
							labelList.Add(BString(label.c_str()));
						}
					
						message->AddInt8("numBars", fConfig.numBars);
						message->AddStrings("labels",labelList);
						/*for (int i = 0; i < fConfig.numBars; i++) {
							std::string indexStr = std::to_string(i);
							message->AddStrings(indexStr.c_str(), fLabels[i].c_str());
						}*/
						TransmitToDaemon(message);
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
							fKillDaemonButton->SetEnabled(true);
							fLaunchDaemonButton->SetEnabled(false);
							fdaemonStatus->SetHighColor(0,200,0,255);
							fSerialPortControl->SetEnabled(true);
							fBacklightSlider->SetEnabled(true);
							fConfigLabelsButton->SetEnabled(true);
							fShowLabelsButton->SetEnabled(true);
							fResetBSButton->SetEnabled(true);
							fnumBarsSlider->SetEnabled(true);
							fBarSettings->Show();
							mainView->Invalidate();
						} else {
							fdaemonStatus->SetText("Not running");
							fKillDaemonButton->SetEnabled(false);
							fLaunchDaemonButton->SetEnabled(true);
							fdaemonStatus->SetHighColor(255,0,0,255);
							fSerialPortControl->SetEnabled(false);
							fBacklightSlider->SetEnabled(false);
							fConfigLabelsButton->SetEnabled(false);
							fShowLabelsButton->SetEnabled(false);
							fResetBSButton->SetEnabled(false);
							fnumBarsSlider->SetEnabled(false);
							fBarSettings->Hide();
							mainView->Invalidate();
						}
						prevStat=status;
					}
				}
				break;
			/*case DAEMON_PING:
				{
					daemonStatus=true;
					BMessage* msg = new BMessage(DAEMON_STATUS);
					msg->AddBool("status",true);
					be_app->WindowAt(0)->PostMessage(msg);
				}
				break;*/
			case LAUNCH_DAEMON:
				system("/boot/home/Documents/Progjets/bargraph/bargraph/BarGraphDaemon &");
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
	BSlider* fnumBarsSlider;
	BTextControl* fshutdownText;
	BStringView* flabelStatus;
	BStringView* fdaemonStatus;
	std::vector<std::string> fLabels;
	BButton* fLaunchDaemonButton;
	
	bool prevStat=false;
    static const uint32 TOGGLE_LABELS = 'SLAB';
    static const uint32 CHANGE_BACKLIGHT = 'SETB';
    static const uint32 SET_LABEL = 'stlb';
	static const uint32 UPDATE_NUM_BARS = 'upnb';
	static const uint32 CONFIGURE_LABELS = 'SCFG';
	static const uint32 REMOTE_QUIT_REQUEST = '_RQR';
	static const uint32 RESET_BARS = 'RSBR';
	static const uint32 SERIAL_PATH = 'SPTH';
	static const uint32 DAEMON_PING = 'PING';
	static const uint32 LAUNCH_DAEMON = 'LNCD';
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
			//BarGraphPrefletWindow* mainwin = ;
			//mainwin->Show();
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
					//be_app->WindowAt(0)->PostMessage(msg);
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
            config.labels = {"1:", "2:", "3:", "4:","F1", "F2", "F3", "F4"}; //implementata M: e F1,F2,Fx
            saveConfig(config);  // Salva la configurazione di default
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