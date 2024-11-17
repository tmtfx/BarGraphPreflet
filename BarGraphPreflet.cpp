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
#include <InterfaceKit.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

const char* kApplicationSignature = "application/x-vnd.BarGraph-Preflet";
const std::vector<std::string> labeloptions = {"1:", "2:", "3:", "4:", "5:", "6:", "7:", "8:", "M:", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8"};

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
        : BWindow(BRect(100, 100, 400, 700), "Bar Graph Settings", B_TITLED_WINDOW, B_NOT_RESIZABLE),
          fConfig(config)
    {
        // Layout
        mainView = new BView(Bounds(), "MainView", B_FOLLOW_ALL, B_WILL_DRAW);
        mainView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

        // Serial Port
        fSerialPortControl = new BTextControl(BRect(10, 10, 290, 30), "SerialPort", "Serial Port:", fConfig.serialPort.c_str(), nullptr);
		
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
		fshutdownText = new BTextControl(BRect(10, 550, 290, 575), "shutdownText", "Shutdown Text:", "", nullptr);
        for (int i = 0; i < fConfig.numBars; i++) {
            BMenu* menu = new BMenu("Select Label");
            //const std::vector<std::string> options = {"1:", "2:", "3:", "4:", "M:", "F1", "F2", "F3", "F4"};
            for (const auto& option : labeloptions) {
				BMessage* msg = new BMessage(SET_LABEL);
				msg->AddInt32("index",i);
				msg->AddString("label",option.c_str());
                BMenuItem* item = new BMenuItem(option.c_str(), msg);
                item->SetTarget(this);
                menu->AddItem(item);
            }
			menu->SetLabelFromMarked(true);
			std::string lbl = "Bar label " +std::to_string(i+1);
            BMenuField* menuField = new BMenuField(BRect(10, 30 * i, 300, 30 * (i + 1)), nullptr, lbl.c_str(), menu);
            fBarSettings->AddChild(menuField);
        }

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

		// Ridisegna la finestra
		fBarSettings->Invalidate();
		mainView->Invalidate();
	}
	
	virtual void MessageReceived(BMessage* message) override {
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
					std::string label = message->FindString("label");
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
	std::vector<std::string> fLabels;
	
    static const uint32 TOGGLE_LABELS = 'SLAB';
    static const uint32 CHANGE_BACKLIGHT = 'SETB';
    static const uint32 SET_LABEL = 'stlb';
	static const uint32 UPDATE_NUM_BARS = 'upnb';
	static const uint32 CONFIGURE_LABELS = 'SCFG';
	static const uint32 REMOTE_QUIT_REQUEST = '_RQR';
	static const uint32 RESET_BARS = 'RSBR';
};


class BarGraphPreflet : public BApplication {
public:
	BarGraphPreflet()
        : BApplication(kApplicationSignature) {
			Config config;
			config = loadConfig();
			BarGraphPrefletWindow* window = new BarGraphPrefletWindow(config);
			window->Show();
			//BarGraphPrefletWindow* mainwin = ;
			//mainwin->Show();
		}
private:
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
};

int
main()
{
	BarGraphPreflet* app = new BarGraphPreflet();
	app->Run();	
	delete app;
	return 0;
}