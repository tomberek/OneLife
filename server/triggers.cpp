#include "triggers.h"
#include "map.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/network/Socket.h"
#include "minorGems/network/SocketClient.h"


#include <stdlib.h>


static char triggersEnabled = false;



typedef struct PlayerMapping {
        const char *email;
        int displayID;
        double startAge;
        GridPos startPos;
        int holdingID;
        ClothingSet clothing;
    } PlayerMapping;

static SimpleVector<PlayerMapping> playerMap;
    


        


typedef struct LiveDummySocket {
        double socketStartTime;
        Socket *sock;
        GridPos pos;
    } LiveDummySocket;


SimpleVector<Socket*> dummySockets;

    




void freeTriggers() {
    triggersEnabled = false;

    for( int i=0; i<dummySockets.size(); i++ ) {
        delete dummySockets.getElementDirect(i);
        }
    dummySockets.deleteAll();
    }


char areTriggersEnabled() {
    return triggersEnabled;
    }



static PlayerMapping *getTriggerPlayer( const char *inPlayerEmail ) {
    for( int i=0; i<playerMap.size(); i++ ) {
        PlayerMapping *p = playerMap.getElement( i );
        
        if( strcmp( p->email, inPlayerEmail ) == 0 ) {
            return p;
            }
        }
    return NULL;
    }

        

int getTriggerPlayerDisplayID( const char *inPlayerEmail ) {
    PlayerMapping *p = getTriggerPlayer( inPlayerEmail );
    
    if( p != NULL ) {
        return p->displayID;
        }
    
    return -1;
    }


double getTriggerPlayerAge( const char *inPlayerEmail ) {
    PlayerMapping *p = getTriggerPlayer( inPlayerEmail );
    
    if( p != NULL ) {
        return p->startAge;
        }

    return -1;
    }



GridPos getTriggerPlayerPos( const char *inPlayerEmail ) {
    PlayerMapping *p = getTriggerPlayer( inPlayerEmail );
    
    if( p != NULL ) {
        return p->startPos;
        }

    GridPos badPos = { -1, -1 };
    return badPos;
    }



int getTriggerPlayerHolding( const char *inPlayerEmail ) {
    PlayerMapping *p = getTriggerPlayer( inPlayerEmail );
    
    if( p != NULL ) {
        return p->holdingID;
        }

    return 0;
    }



ClothingSet getTriggerPlayerClothing( const char *inPlayerEmail ) {
    PlayerMapping *p = getTriggerPlayer( inPlayerEmail );
    
    if( p != NULL ) {
        return p->clothing;
        }

    return getEmptyClothingSet();
    }



static LiveDummySocket newDummyPlayer( const char *inEmail,
                                       int inDisplayID,
                                       double inStartAge,
                                       GridPos inStartPos,
                                       int inHoldingID,
                                       ClothingSet inClothing ) {
    PlayerMapping m = { inEmail, inDisplayID, inStartAge, inStartPos,
                        inHoldingID, inClothing };
    
    playerMap.push_back( m );
    

    LiveDummySocket s;
    s.pos = inStartPos;
    
    s.socketStartTime = Time::getCurrentTime();
    
    int port = SettingsManager::getIntSetting( "port", 8005 );

    HostAddress a( stringDuplicate( "localhost" ), port );

    char timeout;
    s.sock = SocketClient::connectToServer( &a, 100, &timeout );

    if( s.sock != NULL ) {
        dummySockets.push_back( s.sock );
        }
    

    // next send login message

    char *message = autoSprintf( "LOGIN %s dummyHash dummyHash#", inEmail );
    
    int messageLength = strlen( message );
    
    s.sock->send( (unsigned char*)message, 
                      messageLength, 
                      false, false );
    
    delete [] message;
    
    return s;
    }



void sendDummyMove( LiveDummySocket *inDummy, 
                    SimpleVector<GridPos> *inOffsets ) {
    
    char *message = autoSprintf( "MOVE %d %d ", 
                                 inDummy->pos.x, inDummy->pos.y );
    
    
    for( int i=0; i<inOffsets->size(); i++ ) {
        
        char *oldMessage = message;
        
        char *newPart = autoSprintf( "%d %d ",
                                     inOffsets->getElementDirect( i ).x,
                                     inOffsets->getElementDirect( i ).y );
        message = concatonate( message, newPart );
        
        delete [] newPart;
        
        delete [] oldMessage;
        }

    // end pos
    inDummy->pos.x += inOffsets->getElementDirect( inOffsets->size() - 1 ).x;
    inDummy->pos.y += inOffsets->getElementDirect( inOffsets->size() - 1 ).y;
    

    char *oldMessage = message;
    
    message = concatonate( message, "#" );
    
    delete [] oldMessage;

    inDummy->sock->send( (unsigned char*)message, 
                         strlen( message ), 
                         false, false );
    
    delete [] message;
    }


// offset is from current pos
void sendDummyAction( LiveDummySocket *inDummy, 
                      const char *inAction, GridPos inOffset,
                      char inUseExtraA = false, int inExtraA = -1,
                      char inUseExtraB = false, int inExtraB = -1 ) {
    char *message = autoSprintf( "%s %d %d",
                                 inAction, 
                                 inDummy->pos.x + inOffset.x,
                                 inDummy->pos.y + inOffset.y );
    
    if( inUseExtraA ) {
        char *extra = autoSprintf( " %d", inExtraA );
        char *oldMessage = message;
        message = concatonate( message, extra );
        delete [] oldMessage;
        delete [] extra;
        }
    if( inUseExtraB ) {
        char *extra = autoSprintf( " %d", inExtraB );
        char *oldMessage = message;
        message = concatonate( message, extra );
        delete [] oldMessage;
        delete [] extra;
        }
    
    char *oldMessage = message;
    message = concatonate( message, "#" );
    delete [] oldMessage;

    
    inDummy->sock->send( (unsigned char*)message, 
                         strlen( message ), 
                         false, false );
    
    delete [] message;
    }




void sendDummySay( LiveDummySocket *inDummy,
                   const char *inSay ) {
    char *message = autoSprintf( "SAY 0 0 %s#", inSay );
    
    inDummy->sock->send( (unsigned char*)message, 
                         strlen( message ), 
                         false, false );
    
    delete [] message;
    }

                                 







static char moveUsed = false;
static SimpleVector<GridPos> workingMove;


static void addToMove( int inXOffset, int inYOffset ) {
    if( moveUsed ) {
        workingMove.deleteAll();
        moveUsed = false;
        }

    GridPos p = { inXOffset, inYOffset };
    workingMove.push_back( p );
    }


static SimpleVector<GridPos> *finishMove() {
    moveUsed = true;
    return &workingMove;
    }



// include file here that implements
// customInit
// and
// customTrigger
//#include "triggers/testVideo.cpp"
#include "triggers/teaserVideo.cpp"
//#include "triggers/teaserVideoEnd.cpp"
//#include "triggers/test.cpp"



void initTriggers() {
    triggersEnabled = 
        SettingsManager::getIntSetting( "allowTriggerRequests", 0 );
    
    if( triggersEnabled ) {
        
        if( SettingsManager::getIntSetting( "requireClientPassword", 1 )
            ||
            SettingsManager::getIntSetting( "requireTicketServerCheck", 1 ) ) {
            
            printf( "\n\nERROR:  Client-issued triggers enabled "
                    "(allowTriggerRequests in settings folder), but "
                    "requireTicketServerCheck or requireClientPassword "
                    "is enabled in server settings folder.  Exiting.\n\n" );
            exit( 0 );
            }
        
        customInit();
        }
    }



void trigger( int inTriggerNumber ) {
    
    customTrigger( inTriggerNumber );
    }



void stepTriggers() {
    if( !triggersEnabled ) {
        return;
        }
    
    for( int i=0; i<dummySockets.size(); i++ ) {
        Socket *s = dummySockets.getElementDirect( i );
        
        // read all available data and discard it
        
        char buffer[512];
    
        int numRead = 1;
        
        while( numRead > 0 ) {
            numRead = s->receive( (unsigned char*)buffer, 512, 0 );
            }
        }
    
    }


    
    


