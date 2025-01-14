/*
    Focus Lynx INDI driver
    Copyright (C) 2015 Jasem Mutlaq (mutlaqja@ikarustech.com)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

#pragma once

#include "indifocuser.h"

#include <indifocuser.h>
#include <indicom.h>
#include "connectionplugins/connectionserial.h"
#include "connectionplugins/connectiontcp.h"

#include <map>
#include <termios.h>
#include <unistd.h>
#include <memory>
#include <cmath>
#include <cstring>

#define LYNXFOCUS_MAX_RETRIES          1
#define LYNXFOCUS_TIMEOUT              3
#define LYNXFOCUS_MAXBUF               16
#define LYNXFOCUS_TEMPERATURE_FREQ     20      /* Update every 20 POLLMS cycles. For POLLMS 500ms = 10 seconds freq */
#define LYNXFOCUS_POSITION_THRESHOLD    5      /* Only send position updates to client if the diff exceeds 5 steps */

#define FOCUS_SETTINGS_TAB "Settings"
#define FOCUS_STATUS_TAB   "Status"
#define HUB_SETTINGS_TAB "Device"

#define VERSION                 1
#define SUBVERSION              45

class FocusLynxBase : public INDI::Focuser
{
    public:
        FocusLynxBase();
        FocusLynxBase(const char *target);

        enum
        {
            FOCUS_A_COEFF,
            FOCUS_B_COEFF,
            FOCUS_C_COEFF,
            FOCUS_D_COEFF,
            FOCUS_E_COEFF,
            FOCUS_F_COEFF
        };
        typedef enum
        {
            STATUS_MOVING,
            STATUS_HOMING,
            STATUS_HOMED,
            STATUS_FFDETECT,
            STATUS_TMPPROBE,
            STATUS_REMOTEIO,
            STATUS_HNDCTRL,
            STATUS_REVERSE,
            STATUS_UNKNOWN
        } LYNX_STATUS;
        enum
        {
            GOTO_CENTER,
            GOTO_HOME
        };

        virtual bool Handshake() override;
        virtual const char *getDefaultName() override;
        virtual bool initProperties() override;
        virtual void ISGetProperties(const char *dev) override;
        virtual bool updateProperties() override;
        virtual bool saveConfigItems(FILE *fp) override;
        virtual bool loadConfig(bool silent, const char *property) override;

        virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
        virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
        virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) override;

        virtual IPState MoveAbsFocuser(uint32_t targetPosition) override;
        virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks) override;
        virtual IPState MoveFocuser(FocusDirection dir, int speed, uint16_t duration) override;
        virtual bool AbortFocuser() override;
        virtual void TimerHit() override;
        virtual int getVersion(int *major, int *minor, int *sub);

        void setFocusTarget(const char *target);
        const char *getFocusTarget();
        bool checkIfAbsoluteFocuser();
        bool SyncMandatory(bool enable);
        virtual void debugTriggered(bool enable) override;

        // Device
        bool setDeviceType(int index);
        uint32_t DBG_FOCUS;

        // Misc functions
        bool ack();
        bool isResponseOK();

    protected:
        virtual bool SetFocuserMaxPosition(uint32_t ticks) override;
        virtual bool ReverseFocuser(bool enabled) override;
        virtual bool SyncFocuser(uint32_t ticks) override;

        virtual bool SetFocuserBacklash(int32_t steps) override;
        virtual bool SetFocuserBacklashEnabled(bool enabled) override;

        // Move from private to public to validate
        bool configurationComplete;

        // List all supported models
        ISwitch *ModelS;
        ISwitchVectorProperty ModelSP;

        // Led Intensity Value
        INumber LedN[1];
        INumberVectorProperty LedNP;

        // Store version of the firmware from the HUB
        char version[16];

    private:
        uint32_t simPosition;
        uint32_t targetPosition;

        ISState simStatus[8];
        bool simCompensationOn;
        char focusTarget[8];

        std::map<std::string, std::string> lynxModels;

        struct timeval focusMoveStart;
        float focusMoveRequest;

        // Get functions
        bool getFocusConfig();
        bool getFocusStatus();
        bool getFocusTemp();

        // Set functions

        // Position
        bool setStepSize(u_int16_t stepsize);

        // Temperature
        bool setTemperatureCompensation(bool enable);
        bool setTemperatureCompensationMode(char mode);
        bool setTemperatureCompensationCoeff(char mode, int16_t coeff);
        bool setTemperatureInceptions(char mode, int32_t inter);
        bool setTemperatureCompensationOnStart(bool enable);

        // Motion functions
        bool stop();
        bool home();
        bool center();

        // Led level
        bool setLedLevel(int level);

        // Device Nickname
        bool setDeviceNickname(const char *nickname);

        // Misc functions
        bool resetFactory();
        float calcTimeLeft(timeval, float);

        // Properties

        // Set/Get Temperature
        INumber TemperatureN[1];
        INumberVectorProperty TemperatureNP;

        // Enable/Disable temperature compensation
        ISwitch TemperatureCompensateS[2];
        ISwitchVectorProperty TemperatureCompensateSP;

        // Enable/Disable temperature compensation on start
        ISwitch TemperatureCompensateOnStartS[2];
        ISwitchVectorProperty TemperatureCompensateOnStartSP;

        // Temperature Coefficient Mode
        ISwitch TemperatureCompensateModeS[5];
        ISwitchVectorProperty TemperatureCompensateModeSP;

        // Temperature coefficient and Intercept for selected mode
        INumber TemperatureParamN[2];
        INumberVectorProperty TemperatureParamNP;

        // Reset to Factory setting
        ISwitch ResetS[1];
        ISwitchVectorProperty ResetSP;

        // Go to home/center
        ISwitch GotoS[2];
        ISwitchVectorProperty GotoSP;

        // Status indicators
        ILight StatusL[8];
        ILightVectorProperty StatusLP;

        // Max Travel for relative focusers
        //    INumber MaxTravelN[1];
        //    INumberVectorProperty MaxTravelNP;

        // Focuser Step Size
        INumber StepSizeN[1];
        INumberVectorProperty StepSizeNP;

        // Focus name configure in the HUB
        IText HFocusNameT[1] {};
        ITextVectorProperty HFocusNameTP;

        // Request mandatory action of sync from user
        ISwitch SyncMandatoryS[2];
        ISwitchVectorProperty SyncMandatorySP;

        bool isAbsolute;
        bool isSynced;
        bool isHoming;
        // TODO add property for this.
        bool m_HomeOnStart {false};

        static const uint8_t LYNX_MAX { 64 };
};
