// Copyright Hugh Perkins 2015 hughperkins at gmail
//
// This Source Code Form is subject to the terms of the Mozilla Public License, 
// v. 2.0. If a copy of the MPL was not distributed with this file, You can 
// obtain one at http://mozilla.org/MPL/2.0/.

#include "NeuralNet.h"

#include "NetdefToNet.h"

using namespace std;

#undef STATIC
#undef VIRTUAL
#define STATIC
#define VIRTUAL

// string is structured like:
// prefix-nn*(inner)-postfix
// or:
// prefix-nn*inner-postfix
STATIC std::string expandMultipliers( std::string netdef ) {
//    return netdef; // placeholder for now :-P
    int starPos = netdef.find("*");
    if( starPos != (int)string::npos ) {
        int prefixEnd = netdef.rfind("-", starPos);
        string prefix = "";
        string nnString = "";
        if( prefixEnd == (int)string::npos ) {
            prefixEnd = -1;
            nnString = netdef.substr(0, starPos );
        } else {
            prefixEnd--;
            prefix = netdef.substr(0, prefixEnd + 1 );
            cout << "prefix: [" << prefix << "]" << endl;
            nnString = netdef.substr(prefixEnd + 2, starPos - prefixEnd - 2);
        }
        cout << "nnString: [" << nnString << "]" << endl;
        int repeatNum = atoi( nnString);
        cout << "repeatNum " << repeatNum << endl;
        string remainderString = netdef.substr( starPos + 1 );
        cout << "remainderString [" << remainderString << "]" << endl;
        string inner = "";
        string postfix = "";
        if( remainderString.substr(0, 1 ) == "(" ) {
            // need to find other ')', assume not nested for now...
            int rhBracket = remainderString.find(")");
            if( rhBracket == (int)string::npos ) {
                throw runtime_error( "matching bracket not found in " + remainderString );
//                return false;
            }
            inner = remainderString.substr(1, rhBracket - 1 );
            cout << "inner [" << inner << "]" << endl;
            string newRemainder = remainderString.substr(rhBracket + 1);
            cout << "newRemainder [" << newRemainder << "]" << endl;
            if( newRemainder != "" ) {
                if( newRemainder[0] != '-' ) {
                    throw runtime_error( "expect '-' after ')' in " + remainderString );
    //                return false;
                }
                postfix = newRemainder.substr(1);
                cout << "postfix [" << postfix << "]" << endl;
            }
        } else {
            int innerEnd = remainderString.find("-");
            if( innerEnd == (int)string::npos ) {
                innerEnd = remainderString.length();
            } else {
//                innerEnd;
                postfix = remainderString.substr( innerEnd + 1 );
                cout << "postfix [" << postfix << "]" << endl;
            }
            inner = remainderString.substr(0, innerEnd );
            cout << "inner [" << inner << "]" << endl;
//            if( remainderString.find("-") != string::npos ) {
//                sectionEndPos = remainderString.find("-");
//            }
        }
//        return "";
        // if remainderString starts with (, then repeat up to next )
        // otherwise, repeat up to next -
//        int sectionEndPos = remainderString.length();
//        remainderString = 
        string newString = prefix;
        for( int i = 0; i < repeatNum; i++ ) {
            if( newString != "" ) {
                newString += "-";
            }
            newString += expandMultipliers( inner );
        }
        if( postfix != "" ) {
            newString += "-" + expandMultipliers( postfix );
        }
        cout << "multiplied string: " << newString << endl;
        return newString;
    } else {
        return netdef;
    }    
}

STATIC bool NetdefToNet::parseSubstring( NeuralNet *net, std::string substring, bool isLast ) {
    vector<string>splitLayerDef = split( substring, "{" );
    string baseLayerDef = splitLayerDef[0];
//         optionsDef = "";
    vector<string> splitOptionsDef;
    if( splitLayerDef.size() == 2 ) {
        string  optionsDef = split( splitLayerDef[1], "}" )[0];
        splitOptionsDef = split( optionsDef, "," );
    }
    if( baseLayerDef.find("c") != string::npos ) {
        vector<string> splitConvDef = split( baseLayerDef, "c" );
        int numFilters = atoi( splitConvDef[0] );
        int filterSize = atoi( splitConvDef[1] );
        int skip = 0;
        ActivationFunction *fn = new ReluActivation();
        int padZeros = 0;
        for( int i = 0; i < (int)splitOptionsDef.size(); i++ ) {
            string optionDef = splitOptionsDef[i];
            vector<string> splitOptionDef = split( optionDef, "=");
            string optionName = splitOptionDef[0];
            if( splitOptionDef.size() == 2 ) {
                string optionValue = splitOptionDef[1];
                if( optionName == "skip" ) {
                    skip = atoi( optionValue );
                    cout << "got skip: " << skip << endl;
                }
            } else if( splitOptionDef.size() == 1 ) {
                if( optionName == "tanh" ) {
                    fn = new TanhActivation();
                } else if( optionName == "scaledtanh" ) {
                    fn = new ScaledTanhActivation();
                } else if( optionName == "sigmoid" ) {
                    fn = new SigmoidActivation();
                } else if( optionName == "relu" ) {
                    fn = new ReluActivation();
                } else if( optionName == "linear" ) {
                    fn = new LinearActivation();
                } else if( optionName == "padzeros" ) {
                    padZeros = 1;
                } else if( optionName == "z" ) {
                    padZeros = 1;
                } else {
                    cout << "Error: unknown subkey: [" << splitOptionsDef[i] << "]" << endl;
                    return false;
                }
            } else {
                cout << "Error: unknown subkey: [" << splitOptionsDef[i] << "]" << endl;
                return false;
            }
        }
        net->addLayer( ConvolutionalMaker::instance()->numFilters(numFilters)->filterSize(filterSize)->fn( fn )->padZeros( padZeros )->biased() );
    } else if( baseLayerDef.find("mp") != string::npos ) {
        vector<string> splitPoolDef = split( baseLayerDef, "mp" );
        int poolingSize = atoi( splitPoolDef[1] );
        net->addLayer( PoolingMaker::instance()->poolingSize(poolingSize) );
    } else if( baseLayerDef.find("rp") != string::npos ) {
        int patchSize = atoi( split( baseLayerDef, "rp" )[1] );
        net->addLayer( RandomPatchesMaker::instance()->patchSize( patchSize ) );
    } else if( baseLayerDef.find("rt") != string::npos ) {
        int translateSize = atoi( split( baseLayerDef, "rt" )[1] );
        net->addLayer( RandomTranslationsMaker::instance()->translateSize( translateSize ) );
    } else if( baseLayerDef.find("n") != string::npos ) {
        vector<string> fullDef = split( baseLayerDef, "n" );
        int numPlanes = atoi( fullDef[0] );
        ActivationFunction *fn = new TanhActivation();
        if( isLast ) {
            fn = new LinearActivation();
        }
//        int padZeros = 0;
        int biased = 1;
        for( int i = 0; i < (int)splitOptionsDef.size(); i++ ) {
            string optionDef = splitOptionsDef[i];
//                cout << "optionDef: " << optionDef << endl;
            vector<string> splitOptionDef = split( optionDef, "=");
            string optionName = splitOptionDef[0];
            if( splitOptionDef.size() == 1 ) {
                if( optionName == "tanh" ) {
                    fn = new TanhActivation();
                } else if( optionName == "scaledtanh" ) {
                    fn = new ScaledTanhActivation();
                } else if( optionName == "sigmoid" ) {
                    fn = new SigmoidActivation();
                } else if( optionName == "relu" ) {
                    fn = new ReluActivation();
                } else if( optionName == "nobias" ) {
                    biased = 0;
                } else if( optionName == "linear" ) {
                    fn = new LinearActivation();
                } else {
                    cout << "Error: unknown subkey: [" << splitOptionsDef[i] << "]" << endl;
                    return false;
                }
            } else {
                cout << "Error: unknown subkey: [" << splitOptionsDef[i] << "]" << endl;
                return false;
            }
        }
        if( isLast && fn->getDefineName() != "LINEAR" ) {
            cout << "Last fullyconnectedlayer must be linear (because softmax is the 'activationlayer' for this layer)" << endl;
            return false;
        }
        net->addLayer( FullyConnectedMaker::instance()->numPlanes(numPlanes)->imageSize(1)->fn(fn)->biased(biased) );
    } else {
        cout << "network definition " << baseLayerDef << " not recognised" << endl;
        return false;
    }
    return true;
}

STATIC bool NetdefToNet::createNetFromNetdef( NeuralNet *net, std::string netdef ) {
    string netDefLower = toLower( netdef );
    try {
        netDefLower = expandMultipliers( netDefLower );
    } catch( runtime_error &e ) {
        cout << e.what() << endl;
        return false;
    }
    vector<string> splitNetDef = split( netDefLower, "-" );
    if( netdef != "" ) {
        for( int i = 0; i < (int)splitNetDef.size(); i++ ) {
            string thisLayerDef = splitNetDef[i];
            if( !parseSubstring( net, thisLayerDef, i == (int)splitNetDef.size() - 1 ) ) {
                return false;
            }
        }
    }
    net->addLayer( SoftMaxMaker::instance() );
    return true;
}

