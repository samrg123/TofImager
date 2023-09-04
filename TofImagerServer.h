#pragma once

#include "Wifi.h"
#include "ExtendedWifiCommandServer.h"

#include "Banner.h"

#include "resources/woof.h"
#include "resources/potato.h"

class TofImager;

struct TofImagerServer: ExtendedWifiCommandServer {

    static inline constexpr const char* const kSSID = "TofImager";
    static inline constexpr const char* const kPassword = "password";

    static inline constexpr uint16 kServerPort = 1234;

    Wifi wifi;
    TofImager& tofImager;

    static inline constexpr TofImagerServer& Server(const Command& command) {
        return static_cast<TofImagerServer&>(command.server);
    }

    // Note: HelpCallback uses kExtendedCommands array for info so we forward declare it
    // TODO: Forward declare all Callbacks and implement them in a cpp file. Will cleanup
    //       header and allow us to print extended help when a command is specified with invalid args
    static inline void HelpCallback(Command command);

    // TODO: clean this up, this is just a hack for the easter eggs right now!
    template<size_t kHeight, size_t kWidth>
    static inline void showBmp(const uint16(&bmp)[kHeight][kWidth]) {
      
        if(!gLogDisplay) return;

        gLogDisplay->backBuffer.drawRGBBitmap(
            0, 0,
            bmp[0],
            kWidth, kHeight
        );

        gLogDisplay->Draw();
    
        delay(3000);
    }

    // TODO: Add stream 'type' 'on/off' command 
    static inline constexpr ExtendedCommand kExtendedCommands[] = {

        ExtendedCommand {
            
            .name = "hello",
            .summary = "Says hello to the client.",
            .detailedHelp = "Sends the client 'Hello ip:port' where 'ip:port' corresponds to the client's ip and port numbers.",
            .callback = [](Command command) {

                Connection& connection = command.connection;
                connection.client.printf("Hello %s:%d\n", connection.client.remoteIP().toString().c_str(), connection.client.remotePort()); 
            }
        },   

        ExtendedCommand {
            
            .name = "help",
            .summary = "Prints the help menu. If arguments are provided detailed help is printed for each specified command.",                 

            .args = (ExtendedCommand::Arg[]) {
                { .name = "Command...", .type = ExtendedCommand::Arg::TYPE_OPTIONAL }
            },

            .callback = HelpCallback
        },

        ExtendedCommand {
            .name = "woof",
            .summary = "A royal how do you do.",
            .detailedHelp = "In loving memory of Shay 2008-2021.",
            .callback = [](Command command) {
                command.connection.client.println("The Queen says Woof");
                showBmp(kWoof);          
            }
        },

        ExtendedCommand {
            .name = "potato",
            .summary = "A lazy how do you do.",
            .callback = [](Command command) {
                command.connection.client.println("The Potato says Potate");
                showBmp(kPotato);
            }
        },
    };        

    inline TofImagerServer(TofImager& tofImager_)
        : ExtendedWifiCommandServer(ExtendedCommandMap(kExtendedCommands)), 
          tofImager(tofImager_) {

        onConnect.Append([](WifiServer& server, Connection& connection) {
        
            Log("Connected connectionId: %d | connectedClientCount: %d | Client: %s:%d\n", 
                connection.Id(),
                server.ConnectedClientCount(),
                connection.client.remoteIP().toString().c_str(), connection.client.remotePort()
            );
            
            connection.client.printf(
                "Greetings %s:%d | Connected to: %s:%d\n",
                connection.client.remoteIP().toString().c_str(), connection.client.remotePort(),
                connection.client.localIP().toString().c_str(), connection.client.localPort()
            );
        });

        onDisconnect.Append([](WifiServer& server, Connection& connection) {
            Log("Disconnected connectionId: %d | connectedClientCount: %d\n", 
                connection.Id(),
                server.ConnectedClientCount()
            );
        });
    }

    void Init() {

        wifi.HostNetwork(kSSID, kPassword);
        ExtendedWifiCommandServer::Init(kServerPort);

        Log("Hosting Server {\n"
            "\tSSID: %s\n"
            "\tPort: %d\n"
            "}\n",
            kSSID, 
            kServerPort
        );
        
        // // Wait 5s so port number is available on screen
        // delay(5000);
    }
};

void TofImagerServer::HelpCallback(TofImagerServer::Command command) {

    // TODO: pull these functions out into a text formatter that will automatically
    //       adjust indenting and wrap text to banner width

    auto indentNewlines = [](const char* str) {
        
        // TODO: preallocate the size of result to at least fit str 
        String result;
        if(!str) return result;

        for(char c; (c = *str); ++str) {
            result+= c;
            if(c == '\n') result+= '\t';
        }
        return result;
    };

    auto argsString = [](const ExtendedCommand& extendedCommand) {
        
        const ExtendedCommand::Args& args = extendedCommand.args;
        
        int numArgs = args.numArgs; 
        if(!numArgs) return String("None"); 
        
        String result;
        int i = 0;
        while(true) {

            const ExtendedCommand::Arg& arg = args.arg[i];

            if(arg.type == arg.TYPE_OPTIONAL) {
                result = result + '[' + arg.name + ']';
            } else {
                result = result + '<' + arg.name + '>';
            }
            
            if(++i < numArgs) {
                result+= ' ';
            } else {
                return result;
            }
        }
    };

    auto helpSummaryString = [&](const ExtendedCommand& extendedCommand) {

        String result(extendedCommand.name);
        
        constexpr int kLeftColumnWidth = 8;
        for(int i = result.length(); i < kLeftColumnWidth; ++i) {
            result+= ' ';
        }
        
        return result + ": " + extendedCommand.summary;
    };

    auto detailedHelpString = [&](const ExtendedCommand& extendedCommand) {
        
        String result;
        result = result + extendedCommand.name + " {\n"
                 "\tArgs:    " + argsString(extendedCommand) + "\n"
                 "\tSummary: " + extendedCommand.summary + "\n"
                 "\tDetails: " + (extendedCommand.detailedHelp ? indentNewlines(extendedCommand.detailedHelp) : String("None")) + "\n"
                 "}";
        
        return result;
    };

    String result;
    int numArgs = command.numArgs; 
    if(numArgs) {
    
        // print extended Help
        String helpString;
        for(int i = 0; i < numArgs; ++i) {
            const char* arg = command.args[i];
            
            // TODO: switch to binary search once we can enforce ordering on kExtendedCommands 
            for(const ExtendedCommand& extendedCommand : kExtendedCommands) {
                if(!strcmp(arg, extendedCommand.name)) {
                    helpString = detailedHelpString(extendedCommand);
                    break;
                }
            }
            
            if(helpString.isEmpty()) {
                result = result + "Invalid Command: '" + arg + "'\n";  
            } else {
                result = result + helpString + '\n';
                helpString.clear();                         
            }
        }

    } else {

        constexpr auto kBanner = Banner<90>(" Help Menu ", '-');
        result = result + kBanner.buffer + '\n';

        // print Help summary
        for(const ExtendedCommand& extendedCommand : kExtendedCommands) {
            result = result + helpSummaryString(extendedCommand) + '\n';
        }
    }

    Connection& connection = command.connection;
    connection.client.print(result); 
}
