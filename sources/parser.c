//
//  paser.c
//  The parser needs to accept a string from the text input
//
//  Created by ben on 07/11/2014.
//
//



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "../includes/tower.h"
#include "../includes/actionQueueDataStructure.h"
#include "./../includes/parser.h"
#include "../includes/Information_Window.h"
#include "../includes/abilities.h"
#include "../includes/enemy.h"
#include "../includes/sput.h"
#include "../includes/gameProperties.h"
#include "../includes/abilities.h"

#pragma mark ProtoTypes
//top level command parser:
int parseCommands(char ** commandArray, int numberOfTokens);

//specific command parsers:
int parseCat(char * inputStringTargeting);
int parseMan(char * inputStringCommandMan);
int parseAptget(char * aptToGetString);
int parsePs(char * optionString);
int parseKill(char ** commandArray,int numberOfChunks);
int parseMktwr(char ** commandArray, int numberOfTokens);
int parseChmod(char ** commandArray,int numberOfTokens);
int parseUpgrade(char ** commandArray, int numberOfChunks);

void cleanUpParseUpgrade(cmdOption * statsToUpgradeArray,int * targetArray);

//generic functions:
cmdType getCommandType(char * firstToken);
cmdOption getCommandOption(char * input);
unsigned int getTargetTower(const char * inputStringTargeting, bool needsIdentifier);
unsigned int getTargetEnemy(const char * inputStringTargeting);
unsigned long int stringToInt(const char * string);
char ** breakUpString(const char * inputString, int *numberOfChunksPtr, const char * delimiter);
char * strdup(const char * s);
void freeCommandArray(char **commandArray,int numberOfChunks);

//default error messaging functions:
void actionUsageError();

//initialiser structs:
typedef struct stringList {
    char ** stringArray;
    int numberOfStrings;
} stringList;

typedef struct  environmentVariable {
    char * name;
    char * name2;
    int value;
    int (*updateValueFunc)(cmdType command);//updates the value with things on the queue
    int (*getValueFunc)();//sets the value from the global value
} envVar;

typedef struct environmentVariableList {
    envVar ** array;
    int numberOfElements;
} envVarList;

//initialiser functions:
stringList * intialiseCommandList();
stringList * intialiseOptionList();
stringList * getCommandList(stringList * commandList);
stringList * getOptionList(stringList * optionList);
envVarList * intialiseEnvVarsList();
envVarList * getEnvsList(envVarList * envsList);

//parseWhile structs:
typedef struct targetArray {
    int * array;
    int numberOfElements;
} targetArrayStruct;

typedef struct statArray {
    cmdOption * array;
    int numberOfElements;
} statArrayStruct;

typedef struct upgradeArrays {
    statArrayStruct * statArray;
    targetArrayStruct * tarArray;
} upgradeArraysStruct;

typedef enum operator {
    error = -1,
    none = 0,
    not = '!',
    greaterThan = '>',
    lessThan = '<',
    equalTo = '=',
    greaterThanOrEqualTo = '>'+'=',
    lessThanOrEqualTo = '<'+'=',
    sameAs = '='+'=',
    notEqualTo = '!'+'='
} operator;

//While parsing funtions:
int parseWhile(char *inputString);

operator matchesOperator(char isThisAnOperator);
operator combineOperators(operator firstOp, operator secondOp);
operator getOperatorFromString(char * conditionString);
void makeStringForOperator(operator op, char * string);

envVar * returnEnvVar(char * stringToMatch);
int getCommandMemCost(cmdType command, envVar * mem);
int numberOfMktwrLastPushed (int mktwrsPushed);
upgradeArraysStruct * getStatsToUpgradeArrayAndTargetArray(upgradeArraysStruct * upgradeStruct);
int getCostOfMktwr();
int getCostOfUpgrade(upgradeArraysStruct * upgradeStuct);
int updateTowsValue(cmdType command);
int updateMemValue(cmdType command);

int isThisInfiniteLoop(envVar * variable, operator op, char ** commandArray);

//developement testing functions:
void testStringLists();
void testCommandArray(char ** commandArray, int numberOfChunks);

//automated unit tests:

//genericFuntion unit tests:
void testInitialiseParseLists();
void testStringToInt();
void testGetTargetTower();
void testGetTargetEnemy();
void testGetCommandType();
void testGetCommandOption();
void testBreakUpStringAndFreeCommandArray();

//individual command unit tests
void testChmod();
void testParsePs();
void testAptget();
void testKill();
void testParseMktwr();
void testMan();
void testCat();
void testUpgrade();

//parseWhile and associate functions unit tests:
void testGetOperatorFromString();
void testReturnEnvVar();
void testParseWhile();


#pragma mark mainFuntion

/*
 * Parse called with string of user input from terminal window.
 * reads the first token and calls the relevant command function 
 * returns zero if syntax error.
 */
int parse(char *inputString)
{
    commandToTerminalWindow(inputString); //Display input string in terminal window, whether recognized or not
    size_t len = 1+strlen(inputString);//gets the size of inputString
    if( len < 3*sizeof(char) )
    {
        fprintf(stderr,"ERROR: valid commands must be longer than that\n");
        errorToTerminalWindow("ERROR: valid commands must be longer than that");
        return 0;
    }
    if(tolower(inputString[0])=='w' && tolower(inputString[1])=='h' &&
       tolower(inputString[2])=='i' && tolower(inputString[3])=='l' &&
       tolower(inputString[4])=='e')
    {
        return parseWhile(inputString);
    }
    int numberOfTokens;
    char **commandArray = breakUpString(inputString, &numberOfTokens, " ,");
    //testCommandArray(commandArray, numberOfTokens);
    //array of strings, each elem holds a token string from the input command
    int minNumberOfChunks = 2;//as of cat man and upgrade
    if( numberOfTokens < minNumberOfChunks )
    {
        fprintf(stderr,"ERROR: commands must be space seperated, with at least two words\n");
        errorToTerminalWindow("ERROR: commands must be space seperated, with at least two words");
        freeCommandArray(commandArray, numberOfTokens);
        return 0;//no valid commands with less than 2 strings or more than 3
    }
    int specificReturns = parseCommands(commandArray,numberOfTokens);
    
    freeCommandArray(commandArray, numberOfTokens);
    return specificReturns;//0 for error
}
/*
 *
 */
int parseCommands(char ** commandArray, int numberOfTokens)
{
    //enumerated type cmdType can describe each of the possible commands(see actionQueue.h)
    cmdType command = getCommandType(commandArray[0]);//the first string in the command should contain the action
    if(command==cmd_commandError)//if getAction returns commandError then the input is invalid
    {                //Error messaging handled in getCommandType function
        return 0;
    }
    int specificReturns=0;//stores return values of the different functions that execute the commands
    /**** Now we deal with each possible command separately as they all have different syntax ****/
    switch (command)
    {
        case cmd_upgrade:
        {
            if(numberOfTokens<3)
            {
                fprintf(stderr,"ERROR: Upgrade command expected 2 or more arguments\n");
                errorToTerminalWindow("ERROR: Upgrade command expected 2 or more arguments");
                specificReturns = 0;
            }
            else
            {
                specificReturns = parseUpgrade(commandArray, numberOfTokens);
            }
            break;
        }
        case cmd_cat:
        {
            if(numberOfTokens!=2)
            {
                fprintf(stderr,"ERROR: cat command expected 1 argument only \n");
                errorToTerminalWindow("ERROR: cat command expected 1 argument only");
                specificReturns = 0;
            }
            else
            {
                specificReturns = parseCat(commandArray[1]);
            }
            break;
        }
        case cmd_chmod:
        {
            if(numberOfTokens<3)
            {
                fprintf(stderr,"ERROR: chmod command expected 2 or more arguments\n");
                errorToTerminalWindow("ERROR: chmod command expected 2 or more arguments");
                specificReturns = 0;
            }
            specificReturns = parseChmod(commandArray, numberOfTokens);
            break;
        }
        case cmd_man:
        {
            if(numberOfTokens!=2)
            {
                fprintf(stderr,"ERROR: man command expected 1 argument only \n");
                errorToTerminalWindow("ERROR: man command expected 1 argument only");
                specificReturns = 0;
            }
            else
            {
                specificReturns = parseMan(commandArray[1]);
            }
            break;
        }
        case cmd_mktwr:
        {
            if(numberOfTokens<3)
            {
                fprintf(stderr,"ERROR: mktwr command expected 2 or more arguments\n");
                errorToTerminalWindow("ERROR: mktwr command expected 2 or more arguments");
                specificReturns = 0;
            }
            else
            {
                specificReturns = parseMktwr(commandArray,numberOfTokens);
            }
            break;
        }
        case cmd_aptget:
        {
            if(numberOfTokens!=2)
            {
                fprintf(stderr,"ERROR: apt-get command expected 1 argument only \n");
                errorToTerminalWindow("ERROR: apt-get command expected 1 argument only");
                specificReturns = 0;
            }
            else
            {
                specificReturns = parseAptget(commandArray[1]);
            }
            break;
        }
        case cmd_ps:
        {
            if(numberOfTokens!=2)
            {
                fprintf(stderr,"ERROR: ps command expected 1 argument only \n");
                errorToTerminalWindow("ERROR: ps command expected 1 argument only");
                specificReturns = 0;
            }
            else
            {
                specificReturns = parsePs(commandArray[1]);
            }
            break;
        }
        case cmd_kill:
        {
            if(numberOfTokens<2)
            {
                fprintf(stderr,"ERROR: kill command expected atleast 1 argument \n");
                errorToTerminalWindow("ERROR: kill command expected atleast 1 argument ");
                specificReturns = 0;
            }
            specificReturns = parseKill(commandArray, numberOfTokens);
            break;
        }
        case cmd_commandError:
        {
            printf("command was not read\n");
            specificReturns = 0;
            break;
        }
        case cmd_execute:
        {
            printf("command not implemented\n");
            specificReturns = 0;
            break;
        }
        default:
        {
            fprintf(stderr,"\n***command not implemented in parseCommands yet returning***\n");
        }
    }
    return specificReturns;
}

#pragma mark individualCommandParsers

int parseChmod(char ** commandArray,int numberOfTokens)
{
    cmdOption twrType = getCommandOption(commandArray[1]);
    if( !(twrType==mktwr_int || twrType==mktwr_char) )
    {
        fprintf(stderr,"ERROR: chmod expected a type (int, or char) as its first argument\n");
        errorToTerminalWindow("ERROR: chmod expected a type (int, or char) as its first argument");
        return 0;
    }
    
    // get targets
    int atToken = 2;
    int * targetArray = NULL;
    int numberOfTargets = 0;
    while( atToken < numberOfTokens )
    {
        for(int i = 0; commandArray[atToken][i]; i++)
        {
            commandArray[atToken][i] = tolower(commandArray[atToken][i]);
        }

        int target = getTargetTower(commandArray[atToken], false);
        if(target==0)
        {
            fprintf(stderr,"ERROR: chmod expected target tower or list of towers as second argument onwards\n");
            errorToTerminalWindow("ERROR: chmod expected target tower or list of towers as second argument onwards");
            cleanUpParseUpgrade(NULL, targetArray);
            return 0;
        }
        else
        {
            ++numberOfTargets;
            int * tmp = realloc(targetArray, numberOfTargets*sizeof(int));
            if(tmp==NULL)
            {
                fprintf(stderr,"realloc error parser.c parseUpgrade() 2\n");
                exit(1);
            }
            targetArray=tmp;
            targetArray[numberOfTargets-1] = target;
        }
        ++atToken;
    }
    if(!numberOfTargets)
    {
        fprintf(stderr,"ERROR: chmod needs atleast 1 target tower \n");
        errorToTerminalWindow("ERROR: chmod needs atleast 1 target tower ");
        cleanUpParseUpgrade(NULL, targetArray);
        return 0;
    }
    

    for(int tarIter=0; tarIter<numberOfTargets; ++tarIter)
    {
        if(twrType==mktwr_int)
        {
            setTowerType(targetArray[tarIter], INT_TYPE);
            char str[100];
            sprintf(str,">>> changed t%d to int <<<",targetArray[tarIter]);
            printf("\n>>> changed t%d to int <<< \n",targetArray[tarIter]);
            textToTowerMonitor(str);
        }
        if(twrType==mktwr_char)
        {
            setTowerType(targetArray[tarIter], CHAR_TYPE);
            char str[100];
            sprintf(str,">>> changed t%d to char <<<",targetArray[tarIter]);
            printf("\n>>> changed t%d to char <<< \n",targetArray[tarIter]);
            textToTowerMonitor(str);
        }
    }
    cleanUpParseUpgrade(NULL,targetArray);

    return 1;
}
/*
 *
 */
int parseKill(char ** commandArray,int numberOfTokens)
{
    if(!is_ability_unlocked(KILL))
    {
        errorToTerminalWindow("ERROR: kill ability must be installed first");
        fprintf(stderr,"ERROR: kill ability must be installed first\n");
        return 0;
    }
    cmdOption option = getCommandOption(commandArray[1]);
    
    if(option!=kill_minus9 && option!=kill_all)
    {
        char str[100];
        sprintf(str,"ERROR: Could not read first kill argument ""%s"" expected all or -9 ",
                commandArray[1]);
        fprintf(stderr,"%s\n",str);
        errorToTerminalWindow(str);
        return 0;
    }
    if(option==kill_minus9)
    {
        if(numberOfTokens!=3)
        {
            errorToTerminalWindow("ERROR: Expected 3rd argument to kill -9 containing a target enemy ");
            fprintf(stderr,"ERROR: Expected 3rd argument to kill -9 containing a target enemy\n");
            return 0;
        }
        else
        {
            int targetEnemyID = getTargetEnemy(commandArray[2]);//pass 3rd token
            if(targetEnemyID<=0)
            {
                errorToTerminalWindow("ERROR: Expected 3rd argument to kill -9 containing a target enemy ");
                fprintf(stderr,"ERROR: Expected 3rd argument to kill -9 containing a target enemy\n");
                return 0;
            }
            else
            {
                kill_ability(targetEnemyID);
                return 1;
            }
        }
    }
    else if(option==kill_all)
    {
        if(!numberOfTokens==2)
        {
            errorToTerminalWindow("ERROR: too many arguments to kill all, only 2 expected ");
            fprintf(stderr,"ERROR: too many arguments to kill all, only 2 expected \n");
            return 0;
        }
        else
        {
            kill_all_ability();
            return 2;
        }
    }
    else
    {
        errorToTerminalWindow("ERROR: invalid argument to kill command, expected all or -9");
        fprintf(stderr,"ERROR: invalid argument to kill command, expected all or -9\n");
        return 0;
    }
    return 0;
}
/*
 *
 */
int parsePs(char * optionString)
{
    cmdOption option = getCommandOption(optionString);
    if(option != ps_x)
    {
        errorToTerminalWindow("ERROR: ps command expected x as its second argument");
        return 0;
    }
    else
    {
        psx_ability();
        return 1;
    }
}

/*
 *
 */
int parseAptget(char * aptToGetString)
{
    cmdOption aptToGet = getCommandOption(aptToGetString);
    if(aptToGet!=aptget_kill)
    {
        fprintf(stderr,"\nERROR: app not recognised\n");
        fprintf(stderr,"type man aptget to see availible apps\n");
        errorToTerminalWindow("ERROR: app not recognised. Type man aptget to see availible apps");

        return 0;
    }
    if(!is_valid_unlock(KILL))
    {
        fprintf(stderr,"ERROR: that program is already installed\n");
        errorToTerminalWindow("ERROR: that program is already installed");
        return 0;
    }
    else
    {
        if(pushToQueue(getQueue(NULL),cmd_aptget,aptToGet,0)>=1)
        {
            printf("pushing apt-get to queue\n");
            return 1;
        }
        else
        {
            return 0;
        }
    }
}


/*
 *  Called when we read mktwr cmd.
 *  gets tower position and pushes to queue
 *  returns 1 if cmd was probably successfully pushed to queue
 *  returns 0 if definately not succesful or if target or stat call failed
 */
int parseMktwr(char ** commandArray, int numberOfTokens)
{
    cmdOption twrType = getCommandOption(commandArray[1]);
    if( !(twrType==mktwr_int || twrType==mktwr_char) )
    {
        errorToTerminalWindow("ERROR: mktwr expected a type (int, or char) as its first argument");
        return 0;
    }
    int numberOfCommandsPushed = 0;
    
    int token = 2;
    while(token < numberOfTokens)
    {
        if(strlen(commandArray[token])>1)
        {
            char str[100];
            sprintf(str,"ERROR: mktwr expected a target positon A - %c it read %s",maxTowerPositionChar(),commandArray[token]);
            errorToTerminalWindow(str);
            return 0;
        }
        int towerPosition = (int)tolower(commandArray[token][0]) - 'a' + 1;
        if( towerPosition < 1 || towerPosition > maxTowerPosition() )
        {
            char str[100];
            sprintf(str,"ERROR: mktwr expected a target positon A - %c it read %c",maxTowerPositionChar(),commandArray[token][0]);
            errorToTerminalWindow(str);
            return 0;
        }
        if( isTowerPositionAvailable(towerPosition) )
        {
            if(pushToQueue(getQueue(NULL),cmd_mktwr,twrType,towerPosition)>=1)
            {
                
                printf(">>> pushed mktwr to queue <<<\n");
                ++numberOfCommandsPushed;
            }
        }
        else
        {
            char str[100];
            sprintf(str,"ERROR: postion %c already has a tower built there",commandArray[token][0]);
            errorToTerminalWindow(str);
            return 0;
        }
        ++token;
    }
    return numberOfCommandsPushed;
}

/*  calls man printing functions
 *  returns 1 if ok
 returns 0 if error and prints message
 */
int parseMan(char * inputStringCommandMan)
{
    cmdType commandToMan = getCommandType(inputStringCommandMan);
    switch (commandToMan)
    {
        case cmd_upgrade:
        {
            textToTowerMonitor("GENERAL COMMANDS MANUAL: \n\nupgrade\n\nType ""upgrade"" followed by a stat\n( p, r, s, AOEp, AOEr)\nfollowed by a target tower\ne.g. t1, t2, t3...\nExamples:\nupgrade r t2\nupgrade p t3");
            return 1;
        }
        case cmd_cat:
        {
            textToTowerMonitor("GENERAL COMMANDS MANUAL: \n\ncat \n\ntype ""cat"" followed by a target, e.g. t1, t2, t3..., to display the stats of that target\n");
            return 1;
        }
        case cmd_man:
        {
            textToTowerMonitor("GENERAL COMMANDS MANUAL: \n\nman \n\ntype ""man"" followed by a command, e.g. upgrade or cat, to view the manual\nentry for that command\n");
            return 1;//0 for error
        }
        case cmd_ps:
        {
            textToTowerMonitor("GENERAL COMMANDS MANUAL: \n\nps\n\ntype ""ps"" followed by a command\n ( -x\n ) to discover information about one or more enemies\nExamples:\nps -x\n");
            return 1;//0 for error
        }
        case cmd_kill:
        {
            textToTowerMonitor("GENERAL COMMANDS MANUAL: \n\nps\n\ntype ""kill -9"" followed by a target enemyID (eg 6) or *all*\n to kill one or more enemies\nExamples:\nkill -9 7\n kill -9 all");
            return 1;//0 for error
        }
        case cmd_execute:
        {
            //manExecute();
            return 1;
        }
        case cmd_chmod:
        {
            textToTowerMonitor("GENERAL COMMANDS MANUAL: \n\n chmod \n\n changes the type of a tower to INT or CHAR. \n It accepts a list of target towers as arguments following the command and must have a type INT or CHAR as the final argument. eg chmod t1 2 3 INT, chmod t1,2,3,4,5 char etc..");
            return 1;
        }
        case cmd_mktwr:
        {
            textToTowerMonitor("GENERAL COMMANDS MANUAL: \n\n mktwr\n\n Builds a new tower. Accepts a tower type either int or char as the second argument and one or more position to build on. e.g. mktwr int a b c d    or    mktwr char a,b,d,e,g etc..");
            return 1;
        }
        case cmd_aptget:
        {
            apt_get_query();
            return 1;
        }
        default:
        {
            char str[100];
            sprintf(str,"ERROR: command to man not recognised. You entered: %s",inputStringCommandMan);
            errorToTerminalWindow(str);
            return 0;
        }
    }
}



/*
 *  Called when we read cat cmd.
 *  gets target and pushes to info window.
 *  returns 1 if cmd was probably successfully pushed.
 *  returns 0 if definately not succesful or if target call failed.
 */
int parseCat(char * inputStringTargeting)
{
    //looks for tower type target:
    if( (inputStringTargeting[0]=='t' || inputStringTargeting[0]=='T') && strlen(inputStringTargeting)>=2)
    {
        unsigned int targetTower = getTargetTower(inputStringTargeting, true);
        if(targetTower)
        {
            displayTowerInfo(targetTower);//function in Information_Window.c
            return 1;
        }
        else
        {
            char str[100];
            sprintf(str,"ERROR: cat expected a target tower as\nits 2nd argument");
            errorToTerminalWindow(str);
            return 0;
        }
    }
    //can we also cat other things eg enemies?
    //for now
    else
    {
        char str[100];
        sprintf(str,"ERROR: cat expected a target tower as\nits second argument. Enter t1 tower target tower 1.");
        errorToTerminalWindow(str);
        return 0;
    }
}


                               
                               
                               
                               
                               
#pragma mark parseUpgrade

/*
 *  Called when we read upgrade cmd.
 *  gets stat and target and pushes to queue
 *  returns 1 if cmd was probably successfully pushed to queue
 *  returns 0 if definately not succesful or if target or stat call failed
 */
int parseUpgrade(char ** commandArray, int numberOfChunks)
{
    cmdOption * statsToUpgradeArray = NULL;
    int numberOfStatsBeingUpgraded = 0;
    int atToken = 1;
    while(atToken < numberOfChunks)
    {
        cmdOption statToUpgrade = getCommandOption(commandArray[atToken]);
        if(statToUpgrade==-1) break;
        if(statToUpgrade<=0 || statToUpgrade>8)
        {
            if(tolower(commandArray[atToken][0])=='t' || tolower(commandArray[atToken][0])=='-')
            {
                break;//read a target tower
            }
            else
            {
                printf("herer\n\n");

                char str[200];
                sprintf(str,"ERROR: %s is not a valid tower stat.",commandArray[atToken]);
                fprintf(stderr,"%s\n",str);
                errorToTerminalWindow(str);
                return 0;
            }
        }
        ++numberOfStatsBeingUpgraded;
        cmdOption * tmp = realloc(statsToUpgradeArray, numberOfStatsBeingUpgraded*sizeof(cmdOption));
        if(tmp==NULL)
        {
            fprintf(stderr,"realloc error parser.c parseUpgrade() 1 \n");
            exit(1);
        }
        statsToUpgradeArray=tmp;
        statsToUpgradeArray[numberOfStatsBeingUpgraded-1] = statToUpgrade;
        
        ++atToken;
    }
           
    if(!numberOfStatsBeingUpgraded)
    {
        char str[100];
        sprintf(str,"ERROR: upgrade expected a tower stat as its 1st argument");
        errorToTerminalWindow(str);
        fprintf(stderr,"%s\n",str);
        cleanUpParseUpgrade(statsToUpgradeArray,NULL);
        return 0;
    }

    //now get targets
    int * targetArray = NULL;
    int numberOfTargets = 0;
    while( atToken < numberOfChunks)
    {
        int target = getTargetTower(commandArray[atToken], false);
        if(target==0)
        {
            //error messaging done in getTargetTower
            cleanUpParseUpgrade(statsToUpgradeArray, targetArray);
            return 0;
        }
        
        ++numberOfTargets;
        int * tmp = realloc(targetArray, numberOfTargets*sizeof(int));
        if(tmp==NULL)
        {
            fprintf(stderr,"realloc error parser.c parseUpgrade() 2\n");
            exit(1);
        }
        targetArray=tmp;
        targetArray[numberOfTargets-1] = target;
        ++atToken;
    }
    if(!numberOfTargets)
    {
        char str[100];
        sprintf(str,"ERROR: you must specify a tower to upgrade");
        errorToTerminalWindow(str);
        fprintf(stderr,"%s\n",str);
        cleanUpParseUpgrade(statsToUpgradeArray,targetArray);
        return 0;
    }

    for(int statIter=0; statIter<numberOfStatsBeingUpgraded; ++statIter)
    {
        for(int tarIter=0; tarIter<numberOfTargets; ++tarIter)
        {
            if(pushToQueue(getQueue(NULL),cmd_upgrade, statsToUpgradeArray[statIter],
                           targetArray[tarIter])>=1)
            {
                 printf(">>> pushed stat %d to tar %d <<< \n",statsToUpgradeArray[statIter],
                        targetArray[tarIter]);
            }
        }
    }
    
    targetArrayStruct * pushedTargetArray = malloc(sizeof(targetArrayStruct));
    pushedTargetArray->array = targetArray;
    pushedTargetArray->numberOfElements = numberOfTargets;
    
    statArrayStruct * pushedStatArray = malloc(sizeof(statArrayStruct));
    pushedStatArray->array = statsToUpgradeArray;
    pushedStatArray->numberOfElements = numberOfStatsBeingUpgraded;
    
    upgradeArraysStruct * upgradeStruct = malloc(sizeof(upgradeArraysStruct));
    upgradeStruct->statArray = pushedStatArray;
    upgradeStruct->tarArray = pushedTargetArray;

    getStatsToUpgradeArrayAndTargetArray(upgradeStruct);//storeStatsToUpgradeArray and targetArray
    return 1;
}
                               

void cleanUpParseUpgrade(cmdOption * statsToUpgradeArray,int * targetArray)
{
    if(statsToUpgradeArray)
    {
        free(statsToUpgradeArray);
    }
    if(targetArray)
    {
        free(targetArray);
    }
}

upgradeArraysStruct * getStatsToUpgradeArrayAndTargetArray(upgradeArraysStruct * upgradeStruct)
{
    static upgradeArraysStruct * storedUpgradeStuct = NULL;
    if(upgradeStruct!=NULL)
    {
        if(storedUpgradeStuct)
        {
            free(storedUpgradeStuct->tarArray);
            free(storedUpgradeStuct->statArray);
            free(storedUpgradeStuct);
        }
        storedUpgradeStuct = upgradeStruct;
    }
    return storedUpgradeStuct;
}
 




#pragma mark parseWhile
/*
 *  while(mem>0){ command }
 */
int parseWhile(char *inputString)
{
    int numberOfBracketsTokens;
    char ** bracketTokenArray = breakUpString(inputString, &numberOfBracketsTokens, "(){}");
    if(numberOfBracketsTokens<3)
    {
        fprintf(stderr,"ERROR: was expecting condition and command e.g. ""while ( condition ) { command }"" \n");
        errorToTerminalWindow("ERROR: was expecting condition and command e.g. ""while ( condition ) { command }"" ");
        freeCommandArray(bracketTokenArray,numberOfBracketsTokens);
        return 0;
    }
    else
    {
        int numberOfTokensInCommandArray;
        char ** commandArray = breakUpString( bracketTokenArray[2], &numberOfTokensInCommandArray,
                                             " ," );
        cmdType command = getCommandType(commandArray[0]);
        if(command != cmd_upgrade && command != cmd_mktwr)
        {
            fprintf(stderr,"ERROR: You can only use while loops with an upgrade or mktwr command \n");
            errorToTerminalWindow("ERROR: You can only use while loops with an upgrade or mktwr command ");
            freeCommandArray(bracketTokenArray,numberOfBracketsTokens);
            freeCommandArray(commandArray,numberOfTokensInCommandArray);
            return 0;
        }
        operator op = getOperatorFromString( bracketTokenArray[1] );
        int numberOfOperands=0;
        //envVarList * envsListStruct = getEnvsList(NULL);
        envVar * variable;
        if(op==none || op==not)
        {
            char ** conditionArray = breakUpString(bracketTokenArray[1], &numberOfOperands,
                                                   " ,");
            if(numberOfOperands>1)
            {
                fprintf(stderr,"ERROR: was expecting a single argument for condition with no operator or ! operator \n");
                errorToTerminalWindow("ERROR: was expecting a single argument for condition with no operator or ! operator");
                freeCommandArray(bracketTokenArray,numberOfBracketsTokens);
                freeCommandArray(commandArray,numberOfTokensInCommandArray);
                freeCommandArray(conditionArray,numberOfOperands);
                return 0;
            }
            freeCommandArray(conditionArray,numberOfOperands);
            variable = returnEnvVar(bracketTokenArray[1]);
            if(!variable)
            {
                char termErrString[100];
                sprintf(termErrString,"ERROR: use of undeclared variable %s in condition statement",
                        bracketTokenArray[1]);
                fprintf(stderr,"%s \n",termErrString);
                errorToTerminalWindow(termErrString);
                freeCommandArray(bracketTokenArray,numberOfBracketsTokens);
                freeCommandArray(commandArray,numberOfTokensInCommandArray);
                return 0;
            }
            variable->value = variable->getValueFunc();
            if(isThisInfiniteLoop(variable,none,commandArray))
            {
                char termErrString[100];
                sprintf(termErrString,"ERROR: there is no way to break this loop");
                fprintf(stderr,"%s \n",termErrString);
                errorToTerminalWindow(termErrString);
                freeCommandArray(bracketTokenArray,numberOfBracketsTokens);
                freeCommandArray(commandArray,numberOfTokensInCommandArray);
                return 1;
            }
            //error testing done
            //now execute
            if(op==none)
            {
                return 0;
            }
            if(op==not)
            {
                return 0;
            }
            
        }
        else
        {
            char stringForOperator[5];
            makeStringForOperator(op, stringForOperator);
            char ** conditionArray = breakUpString(bracketTokenArray[1], &numberOfOperands,
                                                   stringForOperator);
            if( numberOfOperands!=2 )
            {
                char termErrString[100];
                sprintf(termErrString,"ERROR: was expecting two operands to %s in conditional \n",
                        stringForOperator);
                fprintf(stderr,"%s \n",termErrString);
                errorToTerminalWindow(termErrString);
                
                freeCommandArray(bracketTokenArray,numberOfBracketsTokens);
                freeCommandArray(commandArray,numberOfTokensInCommandArray);
                freeCommandArray(conditionArray,numberOfOperands);
                return 0;
            }
            envVar * variable = returnEnvVar(conditionArray[0]);
            int conditionTokenIs=1;
            if(!variable)
            {
                variable = returnEnvVar(conditionArray[1]);
                if(!variable)
                {
                    char termErrString[100];
                    sprintf(termErrString,"ERROR: use of undeclared variable in condition statement\n");
                    fprintf(stderr,"%s \n",termErrString);
                    errorToTerminalWindow(termErrString);
                    
                    freeCommandArray(bracketTokenArray,numberOfBracketsTokens);
                    freeCommandArray(commandArray,numberOfTokensInCommandArray);
                    freeCommandArray(conditionArray,numberOfOperands);
                    return 0;
                }
                conditionTokenIs=0;
            }
            int condition = stringToInt(conditionArray[conditionTokenIs]);
            if(conditionTokenIs==0)//for now must have var on LHS
            {
                char termErrString[100];
                sprintf(termErrString,"ERROR: please place the variable on the left hand side of the operator.\n");
                fprintf(stderr,"%s \n",termErrString);
                errorToTerminalWindow(termErrString);
                
                freeCommandArray(bracketTokenArray,numberOfBracketsTokens);
                freeCommandArray(commandArray,numberOfTokensInCommandArray);
                freeCommandArray(conditionArray,numberOfOperands);
                return 0;
            }
            if(isThisInfiniteLoop(variable,op,commandArray))
            {
                char termErrString[100];
                sprintf(termErrString,"ERROR: there is no way to break this loop");
                fprintf(stderr,"%s \n",termErrString);
                errorToTerminalWindow(termErrString);
                
                freeCommandArray(bracketTokenArray,numberOfBracketsTokens);
                freeCommandArray(commandArray,numberOfTokensInCommandArray);
                freeCommandArray(conditionArray,numberOfOperands);
                return 0;
            }
            if(op==greaterThan)
            {
                while(variable->value>condition)
                {
                    if( parseCommands(commandArray,numberOfTokensInCommandArray) )
                    {
                        variable->updateValueFunc(command);
                        if(command==cmd_mktwr)
                        {
                            ++commandArray[2][0];//increments the tower position
                        }
                    }
                    else
                    {
                        freeCommandArray(bracketTokenArray,numberOfBracketsTokens);
                        freeCommandArray(commandArray,numberOfTokensInCommandArray);
                        freeCommandArray(conditionArray,numberOfOperands);
                        return 0;
                    }
                }
                freeCommandArray(bracketTokenArray,numberOfBracketsTokens);
                freeCommandArray(commandArray,numberOfTokensInCommandArray);
                freeCommandArray(conditionArray,numberOfOperands);
                return 1;
            }
            if(op==greaterThanOrEqualTo)
            {
                while(variable->value>=condition)
                {
                    if( parseCommands(commandArray,numberOfTokensInCommandArray) )
                    {
                        variable->updateValueFunc(command);
                        if(command==cmd_mktwr)
                        {
                            ++commandArray[2][0];//increments the tower position
                        }
                    }
                    else
                    {
                        freeCommandArray(bracketTokenArray,numberOfBracketsTokens);
                        freeCommandArray(commandArray,numberOfTokensInCommandArray);
                        freeCommandArray(conditionArray,numberOfOperands);
                        return 0;
                    }
                }
                freeCommandArray(bracketTokenArray,numberOfBracketsTokens);
                freeCommandArray(commandArray,numberOfTokensInCommandArray);
                freeCommandArray(conditionArray,numberOfOperands);
                return 1;
            }
            if(op==lessThan)
            {
                while(variable->value<condition)
                {
                    if( parseCommands(commandArray,numberOfTokensInCommandArray) )
                    {
                        variable->updateValueFunc(command);
                        if(command==cmd_mktwr)
                        {
                            ++commandArray[2][0];//increments the tower position
                        }
                    }
                    else
                    {
                        freeCommandArray(bracketTokenArray,numberOfBracketsTokens);
                        freeCommandArray(commandArray,numberOfTokensInCommandArray);
                        freeCommandArray(conditionArray,numberOfOperands);
                        return 0;
                    }
                }
                freeCommandArray(bracketTokenArray,numberOfBracketsTokens);
                freeCommandArray(commandArray,numberOfTokensInCommandArray);
                freeCommandArray(conditionArray,numberOfOperands);
                return 1;
            }
            if(op==lessThanOrEqualTo)
            {
                while(variable->value<=condition)
                {
                    if( parseCommands(commandArray,numberOfTokensInCommandArray) )
                    {
                        variable->updateValueFunc(command);
                        if(command==cmd_mktwr)
                        {
                            ++commandArray[2][0];//increments the tower position
                        }
                    }
                    else
                    {
                        freeCommandArray(bracketTokenArray,numberOfBracketsTokens);
                        freeCommandArray(commandArray,numberOfTokensInCommandArray);
                        freeCommandArray(conditionArray,numberOfOperands);
                        return 0;
                    }
                }
                freeCommandArray(bracketTokenArray,numberOfBracketsTokens);
                freeCommandArray(commandArray,numberOfTokensInCommandArray);
                freeCommandArray(conditionArray,numberOfOperands);
                return 1;
            }
        }
    }
    freeCommandArray(bracketTokenArray,numberOfBracketsTokens);
    return 1;
}

envVar * returnEnvVar(char * stringToMatch)
{
    envVarList * envsListStruct = getEnvsList(NULL);
    for(int i=0; i<envsListStruct->numberOfElements; ++i)
    {
        if(strcmp(stringToMatch,envsListStruct->array[i]->name)==0 ||
           strcmp(stringToMatch,envsListStruct->array[i]->name2)==0 )
        {
            return envsListStruct->array[i];
        }
    }
    return 0;
}

operator getOperatorFromString(char * conditionString)
{
    int i=0;
    operator firstOp, secondOp, totalOp;
    while(conditionString[i]!='\0')
    {
        firstOp =  matchesOperator(conditionString[i]);
        if(firstOp)
        {
            secondOp = matchesOperator(conditionString[i+1]);
            if(secondOp=='=')
            {
                totalOp = combineOperators(firstOp,secondOp);
                return totalOp;
            }
            else
            {
                return firstOp;
            }
        }
        else
        {
            ++i;
        }
    }
    return 0;
}

operator combineOperators(operator firstOp, operator secondOp)
{
    int combinedOpInt;
    operator combinedOp;
    if(!(firstOp==-1) && secondOp == '=')
    {
        combinedOpInt = (int)firstOp + (int)secondOp;
        combinedOp = (operator)combinedOpInt;
        return combinedOp;
    }
    else return firstOp;
}

operator matchesOperator(char isThisAnOperator)
{
    switch (isThisAnOperator)
    {
        case not:                           return not;
        case greaterThan:                   return greaterThan;
        case lessThan:                      return lessThan;
        case equalTo:                       return equalTo;
        default:                            return 0;
    }
}

void makeStringForOperator(operator op, char * string)
{
    switch (op)
    {
        case not:
            string[0] = '!';
            string[1] = '\0';
            return;
        case greaterThan:
            string[0] = '>';
            string[1] = '\0';
            return;
        case lessThan:
            string[0] = '<';
            string[1] = '\0';
            return;
        case equalTo:
            string[0] = '=';
            string[1] = '\0';
            return;
        case greaterThanOrEqualTo:
            string[0] = '>';
            string[1] = '=';
            string[2] = '\0';
            return;
        case lessThanOrEqualTo:
            string[0] = '<';
            string[1] = '=';
            string[2] = '\0';
            return;
        case sameAs:
            string[0] = '=';
            string[1] = '=';
            string[2] = '\0';
            return;
        case notEqualTo:
            string[0] = '!';
            string[1] = '=';
            string[2] = '\0';
            return;
        case none:
            string[0] = 'n';//no op just a while var is none zero
            string[1] = 'o';
            string[2] = 'n';
            string[3] = 'e';
            string[4] = '\0';
            return;
        default:
            string[0] = 'e';
            string[1] = 'r';
            string[2] = 'r';
            string[3] = '\0';
            return;
    }
}


int isThisInfiniteLoop(envVar * variable, operator op, char ** commandArray)
{
    cmdType command = getCommandType(commandArray[0]);
    if(command != cmd_upgrade && command != cmd_mktwr)
    {
        return 1;
    }
    else if( strcmp(variable->name,"mem")==0 )
    {
        if( op!=greaterThanOrEqualTo && op!=greaterThan )
        {
            char termErrString[100];
            sprintf(termErrString,"ERROR: testing against mem value with a operator other than > or >= would lead to an infinite loop\n");
            fprintf(stderr,"%s \n",termErrString);
            errorToTerminalWindow(termErrString);
            return 1;
        }
    }
    else if( strcmp(variable->name,"tows")==0)
    {
        if( command==cmd_upgrade)
        {
            char termErrString[100];
            sprintf(termErrString,"ERROR: upgrade command doesn't change the number of towers so this would be an infinite loop. You should test against your memory instead.\n");
            fprintf(stderr,"%s \n",termErrString);
            errorToTerminalWindow(termErrString);
            return 1;
        }
        if(command==cmd_mktwr)
        {
            if( op!=lessThan && op!=lessThanOrEqualTo )
            {
                char termErrString[100];
                sprintf(termErrString,"ERROR: number of towers can only increase so this would be an infinite loop. You should use < or <= in your test.\n");
                fprintf(stderr,"%s \n",termErrString);
                errorToTerminalWindow(termErrString);
                return 1;
            }
        }
    }
    return 0;
}

int updateTowsValue(cmdType command)
{
    envVar * tows = returnEnvVar("tows");
    if(command == cmd_mktwr)
    {
        tows->value += 1;
        return tows->value;
    }
    else
    {
        return tows->value;
    }
}

int updateMemValue(cmdType command)
{
    envVar * mem = returnEnvVar("mem");
    if(command == cmd_mktwr)
    {
        mem->value -= getCostOfMktwr();
        return mem->value;
    }
    if(command == cmd_upgrade)
    {
        mem->value -= getCostOfUpgrade( getStatsToUpgradeArrayAndTargetArray(NULL) );
        return mem->value;
    }
    return mem->value;//stops warnings
}

int getCostOfMktwr()
{
    return calculateCosts(cmd_mktwr,0,0);
}

int getCostOfUpgrade(upgradeArraysStruct * upgradeStuct)
{
    int costs=0;
    if( upgradeStuct )
    {
        for(int statIter=0; statIter < upgradeStuct->statArray->numberOfElements; ++statIter)
        {
            for(int tarIter=0; tarIter < upgradeStuct->tarArray->numberOfElements; ++tarIter)
            {
                costs += costOfUpgradeFactoringInTheUpgradesOnTheQueue(upgradeStuct->tarArray->array[tarIter],
                                                                       upgradeStuct->statArray->array[statIter]);
            }
        }
    }
    else
    {
        fprintf(stderr," parse while getCommandsCost error exiting\n");
        exit(0);
    }
    return costs;
}




                               
#pragma mark genericFunctions

                               
/* 
 *  Called on cat and upgrade commands with the target specifying token.
    looks at the 2nd char in the string to find an int 1-9 to be the target.
    Note, wont work for anything > 9, would just see 1.
    Will print its own error message.
    Returns TargetTowerID if sucessful
    Returns 0 if error
 */
unsigned int getTargetTower(const char * inputStringTargeting, bool needsIdentifier)
{
    unsigned int numberOfTowers = getNumberOfTowers();//getNumberOfTowers(); this is func in tower.c
    
    size_t len = strlen(inputStringTargeting);//gets the size of string
    if( len<1  || ( needsIdentifier &&  len<2 ) )
    {
        char str[200];
        sprintf(str,"ERROR: You must target a towers with this command\nTo target a tower enter t followed by a number or list of numbers 1 - %d",numberOfTowers);
        errorToTerminalWindow(str);
        fprintf(stderr,"ERROR: You must target a tower with this command \n");
        fprintf(stderr,"to target a tower enter t followed by a number 1 - %d \n",numberOfTowers);
        return 0;
    }
    if ( needsIdentifier )
    {
        if( inputStringTargeting[0]!='t' && inputStringTargeting[0]!='T' )
        {
            char str[200];
            sprintf(str,"ERROR: You must target a towers with this command\nTo target a tower enter t followed by a number or list of numbers 1 - %d",numberOfTowers);
            errorToTerminalWindow(str);
            fprintf(stderr,"ERROR: You must target a towers with this command\n");
            fprintf(stderr,"to target a tower enter t followed by a number or list of numbers 1 - %d \n",numberOfTowers);
            return 0;
        }
    }
    unsigned int targetTower = 0;
    if( inputStringTargeting[0]=='t' || inputStringTargeting[0]=='T' )
    {
        targetTower = stringToInt(inputStringTargeting+1);
    }
    else
    {
        targetTower = stringToInt(inputStringTargeting);
    }
    if(targetTower > numberOfTowers || targetTower < 1 )
    {
        char str[100];
        sprintf(str,"ERROR: target tower does not existYou have only %d towers you entered t%d",numberOfTowers,
                targetTower);
        errorToTerminalWindow(str);
        
        fprintf(stderr,"ERROR: target tower does not exist \n");
        fprintf(stderr,"You have only %d towers you entered t%d\n",
                numberOfTowers,targetTower);
        return 0;
    }
    return targetTower;
}

/*
 *  Called on cat and upgrade commands with the target specifying token.
 */
unsigned int getTargetEnemy(const char * inputStringTargeting)
{
    unsigned int numberOfEnemies = getNumberOfEnemies();// this is func in enemy.c
    
    size_t len = strlen(inputStringTargeting);//gets the size of string
    if( len<(2*sizeof(char)) )
    {
        fprintf(stderr,"ERROR: You must target a enemy with this command to target a tower enter t followed by a number 1 - %d \n",numberOfEnemies);
        char str[200];
        sprintf(str,"ERROR: You must target a enemy with this command to target a tower enter t followed by a number 1 - %d \n",numberOfEnemies);
        errorToTerminalWindow(str);
        return 0;
    }
    if (inputStringTargeting[0]!='e' && inputStringTargeting[0]!='E')
    {
        fprintf(stderr,"ERROR: You must target a enemy with this command to target a tower enter t followed by a number 1 - %d \n",numberOfEnemies);
        char str[200];
        sprintf(str,"ERROR: You must target a enemy with this command to target a tower enter t followed by a number 1 - %d \n",numberOfEnemies);
        errorToTerminalWindow(str);
        return 0;
    }
    
    unsigned int targetEnemyID = (unsigned int) stringToInt(inputStringTargeting+1);
    
    if(targetEnemyID > numberOfEnemies || targetEnemyID<1)
    {
        char str[100];
        fprintf(stderr,"ERROR: target enemy does not exist there are only %d enemies you entered e%d\n",
                numberOfEnemies,targetEnemyID);
        sprintf(str,"ERROR: target enemy does not exist there are only %d enemies you entered e%d\n",
                numberOfEnemies,targetEnemyID);
        
        errorToTerminalWindow(str);
        return 0;
    }
    return targetEnemyID;
}




unsigned long int stringToInt(const char * string)
{
    unsigned long int converted=0;
    size_t length = strlen(string);
    if(length<1)
    {
        fprintf(stderr,"stringToInt was called with a empty string, this will segfault, so it has just returned 0 \n");
        return 0;
    }
    if(string[0]=='-')
    {
        fprintf(stderr,"stringToInt was called a negative number, this not handled, returning 0\n");
        return 0;
    }
    for(int i=0; i<length; ++i)
    {
        converted += (unsigned int)(string[i]-'0') * pow( 10, (length-i-1));
    }
    return converted;
}
/*
 *  Takes the first string of the input command and tests it against the correct
 syntax to find which command they want to execute then returns that command
 as a enum cmdType variable. Returns cmdType correspodning to the
 validated command or a commandError cmdType
 */
cmdType getCommandType(char * firstToken)
{
    for(int i = 0; firstToken[i]; i++)
    {
        firstToken[i] = tolower(firstToken[i]);
    }
    stringList * commandList = getCommandList(NULL);
    //now test the input string against all valid actions
    cmdType command = cmd_commandError;
    for(int i=1; i<=commandList->numberOfStrings; ++i)
    {
        if(strcmp(firstToken,commandList->stringArray[i])==0)//if the string is identical to one of the commands
        {                                        //then action is set to that command
            switch (i)
            {
                case 1:
                    command = cmd_upgrade;
                    break;
                case 2:
                    command = cmd_execute; //NOT IMPLEMENTED
                    break;
                case 3:
                    command = cmd_chmod;
                    break;
                case 4:
                    command = cmd_man;
                    break;
                case 5:
                    command = cmd_cat;
                    break;
                case 6:
                    command = cmd_mktwr;
                    break;
                case 7:
                    command = cmd_ps;
                    break;
                case 8:
                    command = cmd_aptget;
                    break;
                    
                case 9:
                    command = cmd_kill;
                    break;
            }
            break;
        }
    }
    
    if(command==cmd_commandError)//if it is still set to ERROR then the user made a mistake
    {
        actionUsageError(firstToken);
    }
    return command;
}

/*  Called when after we read a command, tests the next token against the
 *  possible options returns the corresponding cmdOption Or
    returns optionError  and calls the optUsageError function
 */
cmdOption getCommandOption(char * secondToken)
{
    for(int i = 0; secondToken[i]; i++)
    {
        secondToken[i] = tolower(secondToken[i]);
    }
    /*first lets get the array of strings that hold all the possible action commands*/
    stringList * optionList = getOptionList(NULL);
    int numberOfOptions=optionList->numberOfStrings;

    //now test the input string against all valid stats
    cmdOption option = optionError;
    for(int i=1; i<=numberOfOptions; ++i)
    {
        if(strcmp(secondToken,optionList->stringArray[i])==0)//if the string is identical to one of the commands
        {                                        //then action is set to that command
            switch (i)
            {
                case 1:
                    option = upgrade_power;
                    break;
                case 2:
                    option = upgrade_range;
                    break;
                case 3:
                    option = upgrade_speed;
                    break;
                case 4:
                    option = upgrade_AOErange;
                    break;
                case 5:
                    option = upgrade_AOEpower;
                    break;
                case 6:
                    option = upgrade_level;
                    break;
                case 7:
                    option = mktwr_int;
                    break;
                case 8:
                    option = mktwr_char;
                    break;
                case 9:
                    option = ps_x;
                    break;
                case 10:
                    option = kill_minus9;
                    break;
                case 11:
                    option = kill_all;
                    break;
                case 12:
                    option = aptget_kill;
                    break;
            }
            break;
        }
    }
    return option;
}










/*
 *   If there was a syntax error in the users command call this function which
     will print usage advice to the terminal window.
 */
void actionUsageError(const char * firstToken)
{
    stringList * commandList = getCommandList(NULL);
    fprintf(stderr,"*** ""%s"" command not recognised ***\n",firstToken);
    fprintf(stderr,"installed commands: \n");
    char str[200];
    sprintf(str," ""%s"" command not recognised",firstToken);
    
    int numberOfCommands=commandList->numberOfStrings;

    for(int i=1; i<=numberOfCommands; ++i)
    {
        fprintf(stderr,"%s\n",commandList->stringArray[i]);
    }
    fprintf(stderr,"\nType man [COMMAND] for usage\n");//we advise them on usage
    //error messages will need to be passed back to the terminal to be printed.
    //hopefully can do this by setting up a custom stream. For now will print to stderr.

}

/*
 *  Takes the input string and breaks into separate words (where there is a 
    space and new string starts) each of these words is stored in the 
    commandArray which is an array of strings
 */
char **breakUpString(const char * inputString, int *numberOfChunksPtr, const char * delimiter)
{
    char    *stringChunk,                       //holds the chunks on the input string as we break it up
            *inputStringDuplicate = strdup(inputString),//duplicate input string for editting
            **commandArray = NULL;              //this will be an array to hold each of the chunk strings
    int     numberOfChunks=0;
    
    //using http://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm
    stringChunk = strtok(inputStringDuplicate, delimiter); // gets the first chunk (up to the first space)
    
    // walk through rest of string
    while( stringChunk != NULL )
    {
        if(strcmp(stringChunk," ")==0 || strcmp(stringChunk,",")==0)
        {
            goto skipToken;
        }
        ++numberOfChunks;
        commandArray=(char **)realloc(commandArray,numberOfChunks*sizeof(char*));//array of strings
        commandArray[numberOfChunks-1]=(char *)malloc((size_t)(strlen(stringChunk)*sizeof(char)+1));
        strcpy(commandArray[numberOfChunks-1],stringChunk);
    skipToken:
        stringChunk = strtok(NULL, delimiter);
    }
    free(inputStringDuplicate);//frees the malloc made in strdup()
                               //$(numberOfChunks) strings now stored in the commandArray
    *numberOfChunksPtr=numberOfChunks;
    return commandArray;
}

/*
 *  Duplicates a string
 */
char *strdup(const char * s)
{
    size_t len = 1+strlen(s);//gets the size of s
    char *p = malloc(len);//allocates a block big enough to hold s
    
    return p ? memcpy(p, s, len) : NULL;//if p is non 0 ie malloc worked, then copy everything in s into p and return p. if p is NULL malloc didnt work so return NULL
}

/*
 *  frees the memory allocated in breakup string funct
 */
void freeCommandArray(char **commandArray,int numberOfChunks)
{
    for(int i=0; i<numberOfChunks; ++i)
    {
        free(commandArray[i]);
    }
    //free(commandArray);
}


#pragma mark commandLists

void initialiseParser()
{
    stringList * commandList = intialiseCommandList();
    getCommandList(commandList);
    stringList * optionList = intialiseOptionList();
    getOptionList(optionList);
    envVarList * envsListStruct = intialiseEnvVarsList();
    getEnvsList(envsListStruct);

}



stringList * intialiseCommandList()
{
    /* make an array of strings to hold all the possible action commands*/
    char **validActions;
    int numberOfActions=9;//have 5 action commands at this time: upgrade, execute, set, man, cat
    validActions=malloc((numberOfActions)*sizeof(char*));//array of $[numberOfActions] strings
    validActions-=1;
    validActions[1]=strdup("upgrade");
    validActions[2]=strdup("execute");
    validActions[3]=strdup("chmod");
    validActions[4]=strdup("man");
    validActions[5]=strdup("cat");
    validActions[6]=strdup("mktwr");
    validActions[7]=strdup("ps");
    validActions[8]=strdup("apt-get");
    validActions[9]=strdup("kill");


    stringList * commandList = malloc(sizeof(stringList));
    commandList->stringArray=validActions;
    commandList->numberOfStrings=numberOfActions;
    
    return commandList;
}
stringList * getCommandList(stringList * commandList)
{
    static stringList * storedCommandList = NULL;
    if(commandList != NULL && storedCommandList == NULL ) {
        storedCommandList = commandList;
    }
    return storedCommandList;
}
stringList * intialiseOptionList()
{
    /*first lets make an array of strings to hold all the possible action commands*/
    char **validOptions;
    int numberOfOptions=14;//have 5 action commands at this time: upgrade, execute, set, man, cat
    validOptions=malloc((numberOfOptions)*sizeof(char*));    //upgrade opts
    validOptions-=1;
    validOptions[1]=strdup("p");
    validOptions[2]=strdup("r");
    validOptions[3]=strdup("s");
    validOptions[4]=strdup("aoer");
    validOptions[5]=strdup("aoep");
    validOptions[6]=strdup("slowp");
    validOptions[7]=strdup("slowd");
    validOptions[8]=strdup("lvl");
    //mktwr opts:
    validOptions[9]=strdup("int");
    validOptions[10]=strdup("char");
    //ps opts:
    validOptions[11]=strdup("-x");
    //kill opts:
    validOptions[12]=strdup("-9");
    validOptions[13]=strdup("all");
    //aptget opts:
    validOptions[14]=strdup("kill");
    
    stringList * optionsList = malloc(sizeof(stringList));
    optionsList->stringArray=validOptions;
    optionsList->numberOfStrings=numberOfOptions;
    
    return optionsList;
}

stringList *  getOptionList(stringList * optionList)
{
    static stringList * storedOptionList = NULL;
    if(optionList != NULL && storedOptionList == NULL ) {
        storedOptionList = optionList;
    }
    return storedOptionList;
}
void freeParseLists()
{
    stringList * commandList = getCommandList(NULL);
    if(commandList)
    {
        for(int i=1; i<=commandList->numberOfStrings; ++i)
        {
            free(commandList->stringArray[i]);
        }
        free(commandList->stringArray+1);
        free(commandList);
    }
    stringList * optsList = getOptionList(NULL);
    if(optsList)
    {
        for(int i=1; i<=optsList->numberOfStrings; ++i)
        {
            free(optsList->stringArray[i]);
        }
        free(optsList->stringArray+1);
        free(optsList);
    }
}

#pragma mark environmentVariables
envVarList * intialiseEnvVarsList()
{
    envVarList * envsListStruct = malloc(sizeof(envVarList));
    envsListStruct->numberOfElements = 2;
    envsListStruct->array = malloc(envsListStruct->numberOfElements*sizeof(envVar *));
    
    envsListStruct->array[0] = malloc(sizeof(envVar));
    envsListStruct->array[0]->name = strdup("memory");
    envsListStruct->array[0]->name2 = strdup("mem");
    envsListStruct->array[0]->getValueFunc = &getAvailableMemory;
    envsListStruct->array[0]->updateValueFunc = &updateMemValue;
    envsListStruct->array[0]->value = getAvailableMemory();
    
    envsListStruct->array[1] =  malloc(sizeof(envVar));
    envsListStruct->array[1]->name = strdup("tows");
    envsListStruct->array[1]->name2 = strdup("towers");
    envsListStruct->array[1]->getValueFunc = (int (*)())&getNumberOfTowers;
    envsListStruct->array[1]->updateValueFunc = &updateTowsValue;
    envsListStruct->array[1]->value = getNumberOfTowers();
    
    return envsListStruct;
}


void destroyEnvVarList()
{
    envVarList * envsListStruct = getEnvsList(NULL);
    while(envsListStruct->numberOfElements > 0)
    {
        free(envsListStruct->array[envsListStruct->numberOfElements-1]->name);
        free(envsListStruct->array[envsListStruct->numberOfElements-1]->name2);
        free(envsListStruct->array[envsListStruct->numberOfElements-1]);
        envsListStruct->numberOfElements -= 1;
    }
    free(envsListStruct->array);
    free(envsListStruct);
}

envVarList * getEnvsList(envVarList * envsList)
{
    static envVarList * storedEnvsList = NULL;
    if(envsList != NULL && storedEnvsList == NULL )
    {
        storedEnvsList = envsList;
    }
    return storedEnvsList;
}

#pragma mark developementTests


/*
 *  Test function for developement. Prints contents of a commandArray
 */
void testCommandArray(char ** commandArray, int numberOfChunks)
{
    for(int i=0; i<numberOfChunks; ++i)
    {
        printf("[%s]   ",commandArray[i]);
    }
    printf("\n");
}

void testStringLists(stringList * list)
{
    for(int i=1; i<list->numberOfStrings; ++i)
    {
        printf("string %d = [",i);
        
        for(int c=0; c<strlen(list->stringArray[i]); ++c)
        {
            printf("%c",list->stringArray[i][c]);
        }
        printf("]\n");
    }
}


#pragma mark unitTests

void testParser()
{
    sput_start_testing();
    sput_set_output_stream(NULL);
    
    
    sput_enter_suite("testInitialiseParseLists");
    sput_run_test(testInitialiseParseLists);
    sput_leave_suite();
    
    sput_enter_suite("testStringToInt");
    sput_run_test(testStringToInt);
    sput_leave_suite();
    
    sput_enter_suite("testGetTargetTower");
    sput_run_test(testGetTargetTower);
    sput_leave_suite();
    
    sput_enter_suite("testGetTargetEnemy");
    sput_run_test(testGetTargetEnemy);
    sput_leave_suite();
    
    sput_enter_suite("testGetCommandType");
    sput_run_test(testGetCommandType);
    sput_leave_suite();
    
    sput_enter_suite("testGetCommandOption");
    sput_run_test(testGetCommandOption);
    sput_leave_suite();
    
    sput_enter_suite("testBreakUpStringAndFreeCommandArray");
    sput_run_test(testBreakUpStringAndFreeCommandArray);
    sput_leave_suite();
    
    sput_enter_suite("testChmod");
    sput_run_test(testChmod);
    sput_leave_suite();
    
    sput_enter_suite("testParsePs");
    sput_run_test(testParsePs);
    sput_leave_suite();
    
    sput_enter_suite("testAptget");
    sput_run_test(testAptget);
    sput_leave_suite();
    
    sput_enter_suite("testKill");
    sput_run_test(testKill);
    sput_leave_suite();

    sput_enter_suite("testParseMktwr");
    sput_run_test(testParseMktwr);
    sput_leave_suite();

    sput_enter_suite("testMan");
    sput_run_test(testMan);
    sput_leave_suite();
    
    sput_enter_suite("testCat");
    sput_run_test(testCat);
    sput_leave_suite();

    sput_enter_suite("testUpgrade");
    sput_run_test(testUpgrade);
    sput_leave_suite();
    
    sput_enter_suite("testReturnEnvVar");
    sput_run_test(testReturnEnvVar);
    sput_leave_suite();
    
    sput_enter_suite("testGetOperatorFromString");
    sput_run_test(testGetOperatorFromString);
    sput_leave_suite();
    
 
    

    /*
    sput_enter_suite("testParseWhile");
    sput_run_test(testParseWhile);
    sput_leave_suite();*/


    
    sput_finish_testing();
}
#pragma mark unitTests_genericFunctions

void testInitialiseParseLists()
{
    //initialiseParser(); this is called in setUpTesting()
    
    stringList * cmdList = getCommandList(NULL);
    sput_fail_unless(cmdList, "getCommandList() should return stringlist pointer");
    testStringLists(cmdList);
    
    stringList * optList = getOptionList(NULL);
    sput_fail_unless(optList, "getOptionList() should return a string list pointer");
    testStringLists(optList);
    
    sput_fail_unless(returnEnvVar("mem"),"returnEnvVar(""mem"") should return the envVar mem");
    sput_fail_unless(returnEnvVar("tows"),"returnEnvVar(""tows"") should return the envVar tows");
}


void testStringToInt()
{
    
    sput_fail_unless(stringToInt("1")==1,"stringToInt(""1"")==1");
    sput_fail_unless(stringToInt("10")==10,"stringToInt(""10"")==10");
    for(int i=0;i<=10000;i+=999)
    {
        char str[15];
        sprintf(str,"%d",i);
        sput_fail_unless(stringToInt(str)==i,str);
    }
    //bad calls
    sput_fail_unless(stringToInt("")==0,"calling with empty string should return 0 and an error message");
    sput_fail_unless(stringToInt("-10")==0,"calling with negative should return 0 and error message");
}



void testGetTargetTower()
{
    createTowerFromPositions(1);
    printf("%d tower built\n",getNumberOfTowers());
    sput_fail_unless(getTargetTower("",true)==0,"calling with empty string should return 0 and error message");
    sput_fail_unless(getTargetTower("",false)==0,"calling with empty string should return 0 and error message");

    sput_fail_unless(getTargetTower("t",true)==0,"calling with just t should return 0 and error message");
    sput_fail_unless(getTargetTower("t",false)==0,"calling with just t should return 0 and error message");
    sput_fail_unless(getTargetTower("T",true)==0,"calling with just T should return 0 and error message");
    sput_fail_unless(getTargetTower("T",false)==0,"calling with just T should return 0 and error message");
    
    sput_fail_unless(getTargetTower("1",true)==0,"calling with just 1 and string should return 0 and error message when needIdentifer is true");
    sput_fail_unless(getTargetTower("1",false)==1,"calling with just 1 and string should return 1  when needIdentifer is false");
    
    sput_fail_unless(getTargetTower("t0",true)==0,"calling with t0  should return 0 and error message");
    sput_fail_unless(getTargetTower("t-1",true)==0,"calling with t-1  should return 0 and error message");
    sput_fail_unless(getTargetTower("t99",true)==0,"calling with t99  should return 0 and error message");
    sput_fail_unless(getTargetTower("t0",false)==0,"calling with t0  should return 0 and error message");
    sput_fail_unless(getTargetTower("t-1",false)==0,"calling with t-1  should return 0 and error message");
    sput_fail_unless(getTargetTower("t99",false)==0,"calling with t0  should return 0 and error message");
    sput_fail_unless(getTargetTower("-1",false)==0,"calling with -1  should return 0 and error message");
    sput_fail_unless(getTargetTower("0",false)==0,"calling with 0  should return 0 and error message");
    sput_fail_unless(getTargetTower("99",false)==0,"calling with 99  should return 0 and error message");
    
    for(int i=2;i<=maxTowerPosition();++i)
    {
        createTowerFromPositions(i);
        char str[5];
        sprintf(str,"%d",i);
        sput_fail_unless(getTargetTower(str,false)==i,"should get target tower correctly");
        sput_fail_unless(getTargetTower(str,true)==0,"should not get target tower since it is requiring identifier");
        
        char Tstr[10]="t";
        strcat(Tstr,str);
        sput_fail_unless(getTargetTower(Tstr,true)==i,"should get target tower since it is requiring identifier and it has identifier");
        sput_fail_unless(getTargetTower(Tstr,false)==i,"should get target tower since it is not requiring identifier and it has identifier");
        printf("tested with : str = %s\nTstr = %s\n\n",str,Tstr);
    }
    
}

void testGetTargetEnemy()
{
    freeAllEnemies();
    createTestEnemy();
    printf("%d enemies \n",getNumberOfEnemies());
    sput_fail_unless(getTargetEnemy("")==0,"calling with empty string should return 0 and error message");
    
    sput_fail_unless(getTargetEnemy("e")==0,"calling with just e should return 0 and error message");
    sput_fail_unless(getTargetEnemy("E")==0,"calling with just E should return 0 and error message");
    
    sput_fail_unless(getTargetEnemy("1")==0,"calling with just 1 should return 0 and error message");
 
    sput_fail_unless(getTargetEnemy("e2")==0,"calling with e2 should fail because there is only one enemy");
    
    for(int i=2; i<=100;++i)
    {
        createTestEnemy();
        char str[5];
        sprintf(str,"%d",i);
        char eStr[10]="e";
        strcat(eStr,str);
        sput_fail_unless(getTargetEnemy(eStr)==i,"should get target enemy");
       
        printf("tested with : str = %s\n",eStr);
        printf("ther are %d enemies \n\n",getNumberOfEnemies());
    }
}

void testGetCommandType()
{
    //should work for all the actual commands:
    sput_fail_unless(getCommandType(strdup("upgrade"))==cmd_upgrade, "calling ""upgrade"" should return cmd_upgrade");
    sput_fail_unless(getCommandType(strdup("chmod"))==cmd_chmod, "calling ""chmod"" should return cmd_chmod");
    sput_fail_unless(getCommandType(strdup("man"))==cmd_man, "calling ""man"" should return cmd_man");
    sput_fail_unless(getCommandType(strdup("cat"))==cmd_cat, "calling ""cat"" should return cmd_cat");
    sput_fail_unless(getCommandType(strdup("mktwr"))==cmd_mktwr, "calling ""mktwr"" should return cmd_mktwr");
    sput_fail_unless(getCommandType(strdup("ps"))==cmd_ps, "calling ""ps"" should return cmd_ps");
    sput_fail_unless(getCommandType(strdup("apt-get"))==cmd_aptget, "calling ""apt-get"" should return cmd_aptget");
    sput_fail_unless(getCommandType(strdup("kill"))==cmd_kill, "calling ""kill"" should return cmd_kill");
    //and should fail for non valid commands:
    sput_fail_unless(getCommandType(strdup("upgradez"))==cmd_commandError, "calling ""upgradez"" should return error");
    sput_fail_unless(getCommandType(strdup("!upgrade"))==cmd_commandError, "calling ""!upgrade"" should return error");
    sput_fail_unless(getCommandType(strdup("-upgrade"))==cmd_commandError, "calling ""-upgrade"" should return error");
    sput_fail_unless(getCommandType(strdup("upgade"))==cmd_commandError, "calling ""upgade"" should return error");
    sput_fail_unless(getCommandType(strdup("upgrad"))==cmd_commandError, "calling ""upgrad"" should return error");
    sput_fail_unless(getCommandType(strdup(""))==cmd_commandError, "calling with empty string should return error");
    sput_fail_unless(getCommandType(strdup("random"))==cmd_commandError, "calling with random string should return error");
    sput_fail_unless(getCommandType(strdup("!@£)!)IR!@*£"))==cmd_commandError, "calling with random string should return error");
    //and that tolower() is working:
    sput_fail_unless(getCommandType(strdup("UPGRADE"))==cmd_upgrade, "calling ""UPGRADE"" should return cmd_upgrade");
}


void testGetCommandOption()
{
    //should work for all the actual options:
    sput_fail_unless(getCommandOption(strdup("p"))==upgrade_power, "calling ""p"" should return upgrade_power");
    sput_fail_unless(getCommandOption(strdup("r"))==upgrade_range, "calling ""r"" should return upgrade_range");
    sput_fail_unless(getCommandOption(strdup("s"))==upgrade_speed, "calling ""s"" should return upgrade_speed");
    sput_fail_unless(getCommandOption(strdup("aoer"))==upgrade_AOErange, "calling ""aoer"" should return upgrade_AOErange");
    sput_fail_unless(getCommandOption(strdup("aoep"))==upgrade_AOEpower, "calling ""aoep"" should return upgrade_AOEpower");
    sput_fail_unless(getCommandOption(strdup("lvl"))==upgrade_level, "calling ""lvl"" should return upgrade_level");
    sput_fail_unless(getCommandOption(strdup("int"))==mktwr_int, "calling ""int"" should return mktwr_int");
    sput_fail_unless(getCommandOption(strdup("char"))==mktwr_char, "calling ""char"" should return mktwr_char");
    sput_fail_unless(getCommandOption(strdup("-x"))==ps_x, "calling ""-x"" should return ps_x");
    sput_fail_unless(getCommandOption(strdup("-9"))==kill_minus9, "calling ""-9"" should return kill_minus9");
    sput_fail_unless(getCommandOption(strdup("all"))==kill_all, "calling ""all"" should return kill_all");
    sput_fail_unless(getCommandOption(strdup("kill"))==aptget_kill, "calling ""kill"" should return aptget_kill");

    //and should fail for non valid commands:
    sput_fail_unless(getCommandOption(strdup("pp"))==optionError, "calling ""pp"" should return error");
    sput_fail_unless(getCommandOption(strdup("z"))==optionError, "calling ""z"" should return error");
    sput_fail_unless(getCommandOption(strdup(""))==optionError, "calling empty string should return error");
    sput_fail_unless(getCommandOption(strdup("-239"))==optionError, "calling ""-239"" should return error");
    sput_fail_unless(getCommandOption(strdup("random"))==optionError, "calling with random string should return error");
    sput_fail_unless(getCommandOption(strdup("!@£)!)IR!@*£"))==optionError, "calling with random string should return error");
    //and that tolower() is working:
    sput_fail_unless(getCommandOption(strdup("P"))==upgrade_power, "calling ""P"" should return upgrade_power");
}


void testBreakUpStringAndFreeCommandArray()
{
    int numberOfTokesTest1=0;
    char ** test1 = breakUpString("break this,string up",&numberOfTokesTest1,", ");
    testCommandArray(test1,numberOfTokesTest1);
    sput_fail_unless(!strcmp(test1[0],"break")  &&
                     !strcmp(test1[1],"this")   &&
                     !strcmp(test1[2],"string") &&
                     !strcmp(test1[3],"up")     &&
                     numberOfTokesTest1==4 ,
    "tested with ""break this,string up"" and space & comma delimeters, should return each seperate word");
    freeCommandArray(test1,numberOfTokesTest1);
    
    test1 = breakUpString("{break}(this){string}(up)",&numberOfTokesTest1,"{}()");
    testCommandArray(test1,numberOfTokesTest1);
    sput_fail_unless(!strcmp(test1[0],"break")  &&
                     !strcmp(test1[1],"this")   &&
                     !strcmp(test1[2],"string") &&
                     !strcmp(test1[3],"up")     &&
                     numberOfTokesTest1==4,
    "tested with ""{break}(this){string}(up)"" and ""{}()"" delim, should return each seperate word");
    freeCommandArray(test1,numberOfTokesTest1);

    test1 = breakUpString("{break} (this),{string} (up)",&numberOfTokesTest1,"{}()");
    testCommandArray(test1,numberOfTokesTest1);
    sput_fail_unless(!strcmp(test1[0],"break")  &&
                     !strcmp(test1[1],"this")   &&
                     !strcmp(test1[2],"string") &&
                     !strcmp(test1[3],"up")     &&
                     numberOfTokesTest1==4,
                     "tested with ""{break} (this),{string} (up)"" and ""{}()"" delim, should return each seperate word");

}

#pragma mark test_individualCommandParses



void testChmod()
{
    sput_fail_unless( parse("chmod")==0, "parse(""chmod"")==0, not enough tokens -> error message and return 0");
    
    sput_fail_unless( parse("chmod char")==0, "parse(""chmod char"")==0, not enough tokens -> error message and return 0");
    
    for(int i=1;i<=maxTowerPosition();++i)
    {
        createTowerFromPositions(i);
    }
    
    sput_fail_unless( parse("chmod p t1")==0, "parse(""chmod p t1"")==0, p is not a twr type -> error message and return 0");
    
    sput_fail_unless( parse("chmod kill t1")==0, "parse(""chmod kill t1"")==0, kill is not a twr type -> error message and return 0");
    
    sput_fail_unless( parse("chmod int t")==0, "parse(""chmod int t"")==0, t does not specify a tower -> error message and return 0");
    
    sput_fail_unless( parse("chmod char t")==0, "parse(""chmod char t"")==0, t does not specify a tower -> error message and return 0");
    
    sput_fail_unless( parse("chmod int t")==0, "parse(""chmod int t"")==0, t does not specify a tower -> error message and return 0");
    
    sput_fail_unless( parse("chmod int t1")==1, "parse(""chmod int t1"")==1, correct syntax -> execution message and return 1");
    
    sput_fail_unless( parse("chmod char t1")==1, "parse(""chmod char t1"")==1, correct syntax -> execution message and return 1");
    
    sput_fail_unless( parse("chmod char 1")==1, "parse(""chmod char 1"")==1, correct syntax -> execution message and return 1");
     sput_fail_unless( parse("chmod int 1")==1, "parse(""chmod int 1"")==1, correct syntax -> execution message and return 1");
    
    sput_fail_unless( parse("chmod int 1 2 s 4")==0, "parse(""chmod int 1 2 s 4"")==0, s does not specify a tower -> error message and return 0");
    
     sput_fail_unless( parse("chmod int 1 2 4 s")==0, "parse(""chmod int 1 2 4 s"")==0, s does not specify a tower -> error message and return 0");
    
     sput_fail_unless( parse("chmod int 1 2' 4")==0, "parse(""chmod int 1 2' 4"")==0, 2' does not specify a tower -> error message and return 0");
    
     sput_fail_unless( parse("chmod int 1 2 ' 4")==0, "parse(""chmod int 1 2 ' 4"")==0, ' does not specify a tower -> error message and return 0");
    
     sput_fail_unless( parse("chmod int t1,2,4")==1, "parse(""chmod int t1,2,4"")==1, correct syntax  ->  execution message and return 1");
    
     sput_fail_unless( parse("chmod int 144 2 4")==0, "parse(""chmod int 144 2 4"")==0, 144 is larger than the number of towers -> error message and return 0");
    
    sput_fail_unless( parse("chmod int t1,t2,t4")==1, "parse(""chmod int t1,t2,t4"")==1, correct syntax  ->  execution message and return 1");
    
     sput_fail_unless( parse("chmod int t1 t2 t4")==1, "parse(""chmod int t1 t2 t4"")==1, correct syntax  ->  execution message and return 1");
}





void testParsePs()
{
    sput_fail_unless(parse("ps")==0, "parse(""ps"")==0, no argument -> error message and return 0");
    sput_fail_unless(parse("ps b s")==0, "parse(""ps b s"")==0, too many arguments -> error message and return 0");
    sput_fail_unless(parse("ps b")==0, "parse(""ps b"")==0, b is not valid argument -> error message and return 0");
    sput_fail_unless(parse("ps p")==0, "parse(""ps p"")==0, p is valid argument but not valid argument of ps command -> error message and return 0");
    //command cannot run in test mode
    //sput_fail_unless(parse("ps -x")==1, "parse(""ps -x"")==1, correct syntax -> call psx_ability() return 1");
    sput_fail_unless(parse("ps -x s")==0, "parse(""ps -x s"")==0, too many arguments -> error message and return 0");
}


void testAptget()
{
    init_abilities();
    sput_fail_unless(parse("apt-get")==0, "parse(""apt-get"")==0, no argument -> error message and return 0");
    sput_fail_unless(parse("apt-get kill n")==0, "parse(""apt-get kill n"")==0, too many arguments -> error message and return 0");
    sput_fail_unless(parse("apt-get b")==0, "parse(""apt-get b"")==0, b is not valid argument -> error message and return 0");
    sput_fail_unless(parse("apt-get p")==0, "parse(""apt-get p"")==0, p is valid argument but not valid argument of apt-get command -> error message and return 0");
    sput_fail_unless(parse("apt-get kill")==1, "parse(""apt-get kill"")==1, correct syntax -> pushes request to queue and return 1");
    unlock_ability(KILL);
    sput_fail_unless(parse("apt-get kill")==0, "parse(""apt-get kill"")==0, kill is already installed -> error message and return 0");
}
/*
 *
 *
int parseKill(char ** commandArray,int numberOfTokens)
{
    cmdOption option = getCommandOption(commandArray[1]);
    
    if(option!=kill_minus9 && option!=kill_all)
    {
        char str[100];
        sprintf(str,"ERROR: Could not read first kill argument ""%s"" expected all or -9 ",
                commandArray[1]);
        fprintf(stderr,"%s\n",str);
        errorToTerminalWindow(str);
        return 0;
    }
    if(option==kill_minus9)
    {
        if(numberOfTokens!=3)
        {
            errorToTerminalWindow("ERROR: Expected 3rd argument to kill -9 containing a target enemy ");
            fprintf(stderr,"ERROR: Expected 3rd argument to kill -9 containing a target enemy\n");
            return 0;
        }
        else
        {
            int targetEnemyID = getTargetEnemy(commandArray[2]);//pass 3rd token
            if(targetEnemyID<=0)
            {
                errorToTerminalWindow("ERROR: Expected 3rd argument to kill -9 containing a target enemy ");
                fprintf(stderr,"ERROR: Expected 3rd argument to kill -9 containing a target enemy\n");
                return 0;
            }
            else
            {
                kill_ability(targetEnemyID);
                return 1;
            }
        }
    }
    else if(option==kill_all)
    {
        if(numberOfTokens!=2)
        {
            errorToTerminalWindow("ERROR: too many arguments to kill all ");
            fprintf(stderr,"ERROR: too many arguments to kill all \n");
            return 0;
        }
        kill_all_ability();
        
        return 2;
    }
    else
    {
        errorToTerminalWindow("ERROR: invalid argument to kill command, expected all or -9");
    }
    return 0;
}*/

void testKill()
{
    init_abilities();
    freeAllEnemies();
    createTestEnemy();
    sput_fail_unless(parse("kill -9 e1")==0,"kill is not yet installed -> error message and return 0");
    unlock_ability(KILL);
    sput_fail_unless(parse("kill 9 e1")==0,"""kill 9 e1"" option is not valid , needs preceeding minus -> error message and return 0");
    sput_fail_unless(parse("kill -9 e1")==1,"""kill -9 e1"" is valid and unlcoked -> return 1");
    sput_fail_unless(parse("kill -9 e")==0,"""kill -9 e"" is invalid target -> error message & return 0");
    sput_fail_unless(parse("kill -9 e")==0,"""kill -9 e"" is invalid target -> error message & return 0");
    
    sput_fail_unless(parse("kill aoer")==0,"""kill aoer"" is invalid option -> error message & return 0");
    sput_fail_unless(parse("kill all")==2,"""kill all"" is valid  -> return 2");
    sput_fail_unless(parse("kill all aoep")==2,"""kill all aoep"" is invalid, too many tokens -> error message return 0");
}


void testParseMktwr()
{
    //clear any towers so that we can make more
    freeAllTowers();
    iprint(getNumberOfTowers());
    cprint(maxTowerPositionChar());

    sput_fail_unless(parse("mktwr char")==0,
                     "parse(""mktwr char"")==0, too few arguments -> error message and return 0");
    sput_fail_unless(parse("mktwr p U")==0,
                     "parse(""mktwr p t1"")==0, p is valid command option but not for mktwr -> error message and return 0");
    sput_fail_unless(parse("mktwr int Us")==0,
                     "parse(""mktwr int Us"")==0,Us is invalid twr position -> error message and return 0");
    sput_fail_unless(parse("mktwr int {")==0,
                     "parse(""mktwr int {"")==0,{ is invalid twr position -> error message and return 0");
    sput_fail_unless(parse("mktwr int 1")==0,
                     "parse(""mktwr int 1"")==0,1 is invalid twr position -> error message and return 0");
    
    for(char twrPosition = 'a'; twrPosition<=tolower(maxTowerPositionChar()); ++twrPosition)
    {
        char str[100];
        sprintf(str,"mktwr int %c",twrPosition);
        printf("\ntesting:: %s\n",str);
        sput_fail_unless(parse(str)==1,
                         "parse(""mktwr int %c"")==1, is valid command -> push 1 tower and return 1");
        createTowerFromPositions((int)tolower(twrPosition)-'a'+1);
        iprint(isTowerPositionAvailable((int)tolower(twrPosition)-'a'+1));
        sput_fail_unless(parse(str)==0,
                         "parse(""mktwr int %c"")==0, building on same location again is invalid -> print error and return 0");
    }
    
    freeAllTowers();
    char cmd[200]="mktwr char ";
    for(char twrPosition = 'a'; twrPosition<=tolower(maxTowerPositionChar()); ++twrPosition)
    {
        char positions[10];
        sprintf(positions,"%c ",twrPosition);
        strcat(cmd,positions);
    }
    printf("testing %s\n",cmd);
    sput_fail_unless(parse(cmd)==maxTowerPosition(),
                     "parse(""mktwr int a b c d e f g...maxTowerPositionChar()"")==maxTowerPosition(), is valid command -> psuh maxTowerPosition() towers and return maxTowerPosition()");
}

void testMan()
{
    sput_fail_unless(parse("man s sd")==0,"parse(""man s sd"")=0, too many arguments -> error message and return 0");
    sput_fail_unless(parse("man test")==0,"parse(""man test"")=0, test not valid argument -> error message and return 0");
    sput_fail_unless(parse("man int")==0,"parse(""man int"")=0, test not valid argument -> error message and return 0");
    char validManStringStart[200]="man ";
    stringList * commandList = getCommandList(NULL);
    for(int cmds = 1; cmds<=commandList->numberOfStrings; ++cmds)
    {
        char * validmanStrings = strdup(validManStringStart);
        strcat(validmanStrings,commandList->stringArray[cmds]);
        sput_fail_unless(parse(validmanStrings)==1,"valid parse -> should print the manual entry and return 1");
    }
}


void testCat()
{
    sput_fail_unless(parse("cat foo bar")==0,"too many arguments -> error message and return 0");
    sput_fail_unless(parse("cat e1")==0,"command must target a tower -> error message and return 0");
    sput_fail_unless(parse("cat t0")==0,"t0 is not a valid tower -> error message and return 0");
    sput_fail_unless(parse("cat t0")==0,"t0 is not a valid tower -> error message and return 0");
    createTowerFromPositions(1);
    sput_fail_unless(parse("cat t1")==1,"cat t1 is valid -> return 1");
}

void testUpgrade()
{
    freeAllTowers();
    char validUpgradeString[300]="upgrade p s r aoer aoep t";
    for(int pos = 1; pos<=maxTowerPosition();++pos)
    {
        createTowerFromPositions(pos);
        char posChar[5];
        sprintf(posChar,"%d ",pos);
        strcat(validUpgradeString,posChar);
    }
    printf("\ntesting: %s\n",validUpgradeString);
    sput_fail_unless(parse(validUpgradeString),"Valid test of all stats on all towers -> return 1");
    upgradeArraysStruct * upgradeStruct = getStatsToUpgradeArrayAndTargetArray(NULL);
    sput_fail_unless(upgradeStruct->statArray->array[0]==upgrade_power &&
                     upgradeStruct->statArray->array[1]==upgrade_speed &&
                     upgradeStruct->statArray->array[2]==upgrade_range &&
                     upgradeStruct->statArray->array[3]==upgrade_AOErange &&
                     upgradeStruct->statArray->array[4]==upgrade_AOEpower,
                     "upgradeStruct->statArray should contain each of the stats pushed ");
    for(int pos = 1; pos<=maxTowerPosition();++pos)
    {
        sput_fail_unless(upgradeStruct->tarArray->array[pos-1]==pos,
                         "upgradeStruct->tarArray should contain each of the targets");
    }
    sput_fail_unless(parse("upgrade p")==0,"not enough arguments -> error message and return 0");
    sput_fail_unless(parse("upgrade p p")==0,"no target tower -> error message and return 0");
    sput_fail_unless(parse("upgrade int t1")==0,"int is not a valid argument to upgrade -> error message and return 0");
    sput_fail_unless(parse("upgrade p t1")==1,"parse(""upgrade -p t1"") valid command -> return 1");
    sput_fail_unless(parse("upgrade p s t1")==1,"parse(""upgrade -p s -t1"") valid command -> return 1");
    sput_fail_unless(parse("upgrade t1 2")==0,"parse(""upgrade t1 2"") no stats  -> error message & return 0");
    sput_fail_unless(parse("upgrade t1 s p ")==0,"parse(""upgrade t1 s p "") stats and targets wrong way round -> return 0");
}


#pragma mark test parseWhile functions

void testReturnEnvVar()
{
    envVar * returned = returnEnvVar("tows");
    printf("returned = %s\n",returned->name);
    sput_fail_unless(strcmp(returned->name,"tows")==0,"check that it returns tows correctly when  returnEnvVar(""tows"") is called");
    returned = returnEnvVar("towers");
    printf("returned = %s\n",returned->name);
    sput_fail_unless(strcmp(returned->name,"tows")==0,"check that it returns tows correctly when  returnEnvVar(""towers"") is called");
    returned = returnEnvVar("mem");
    printf("returned = %s\n",returned->name);
    sput_fail_unless(strcmp(returned->name,"memory")==0,"check that it returns mem correctly when  returnEnvVar(""mem"") is called");
    returned = returnEnvVar("memory");
    printf("returned = %s\n",returned->name);
    sput_fail_unless(strcmp(returned->name,"memory")==0,"check that it returns mem correctly when  returnEnvVar(""memory"") is called");
}

void testGetOperatorFromString()
{
    char str[20];

    operator op = getOperatorFromString("mem>0");
    makeStringForOperator(op,str);
    printf("op = %s\n",str);
    sput_fail_unless(op==greaterThan,"check that correct operator is extracted from mem>0");
    
    op = getOperatorFromString("mem>=0");
    makeStringForOperator(op,str);
    printf("op = %s\n",str);
    sput_fail_unless(op==greaterThanOrEqualTo,"check that correct operator is extracted from mem>=0");
    
    op = getOperatorFromString("tows<5");
    makeStringForOperator(op,str);
    printf("op = %s\n",str);
    sput_fail_unless(op==lessThan,"check that correct operator is extracted from tows<5");
    
    op = getOperatorFromString("tows<=5");
    makeStringForOperator(op,str);
    printf("op = %s\n",str);
    sput_fail_unless(op==lessThanOrEqualTo,"check that correct operator is extracted from tows<=5");
    
    op = getOperatorFromString("tows");
    makeStringForOperator(op,str);
    printf("op = %s\n",str);
    sput_fail_unless(op==none,"check that correct operator is extracted from tows");
    
    op = getOperatorFromString("!tows");
    makeStringForOperator(op,str);
    printf("op = %s\n",str);
    sput_fail_unless(op==not,"check that correct operator is extracted from !tows");
    
    op = getOperatorFromString("tows!=0");
    makeStringForOperator(op,str);
    printf("op = %s\n",str);
    sput_fail_unless(op==notEqualTo,"check that correct operator is extracted from tows!=0");
    
    op = getOperatorFromString("tows==20");
    makeStringForOperator(op,str);
    printf("op = %s\n",str);
    sput_fail_unless(op==sameAs,"check that correct operator is extracted from tows==20");
    
    op = getOperatorFromString("m esd'0   213sd;;w w[p >  2] [ 2'e1] p4  '@$@  $%1%^&");
    makeStringForOperator(op,str);
    printf("op = %s\n",str);
    sput_fail_unless(op==greaterThan,"check that bad input wont cause an issue");
}
/*
 int updateMemValue(cmdType command)
 {
    envVar * mem = returnEnvVar("mem");
    if(command == cmd_mktwr)
    {
        mem->value -= getCostOfMktwr();
        return mem->value;
    }
    if(command == cmd_upgrade)
    {
        mem->value -= getCostOfUpgrade( getStatsToUpgradeArrayAndTargetArray(NULL) );
        return mem->value;
    }
    return mem->value;//stops warnings
 }
 */
void testMemValueFunctions()
{
    envVar * mem = returnEnvVar("mem");
    setMemory(1000);
    mem->getValueFunc();
    sput_fail_unless(mem->value==1000,");

    updateMemValue(cmd_mktwr);
}
void testParseWhile()
{
    freeAllTowers();
    createTowerFromPositions(1);
    sput_fail_unless(parse("while(upgrade p t1)")==0,"parse(""while(upgrade p t1)"")==1. no condition statement -> error message and return 0");
    sput_fail_unless(parse("while(ssz)(upgrade p t1)")==0,"invalid condition -> error message and return 0");
    sput_fail_unless(parse("while(ssz>10)(upgrade p t1)")==0,"invalid variable -> error message and return 0");
    sput_fail_unless(parse("while(mem)(upgrade p t1)")==0,"invalid condition -> error message and return 0");
    sput_fail_unless(parse("while(tows>0)(upgrade p t1)")==0,"invalid, breakInfinite loop called -> error message and return 0");
    sput_fail_unless(parse("while(mem<10)(upgrade p t1)")==0,"invalid, breakInfinite loop called -> error message and return 0");
     sput_fail_unless(parse("while(mem<=10)(upgrade p t1)")==0,"invalid, breakInfinite loop called -> error message and return 0");
    sput_fail_unless(parse("while(tows>=10)(mktwr int b)")==0,"invalid, breakInfinite loop called -> error message and return 0");
    sput_fail_unless(parse("while(tows>10)(mktwr int b)")==0,"invalid, breakInfinite loop called -> error message and return 0");
    sput_fail_unless(parse("while(tows<10)(cat t1)")==0,"invalid, breakInfinite loop called -> error message and return 0");
    sput_fail_unless(parse("while(tows<10)(try with invalid command)")==0,"invalid, no valid command -> error message and return 0");
    
    sput_fail_unless(parse("while ( mem > 10 ) ( try with invalid command )")==0,"invalid, no valid command. and make sure spaces in between the brackets causes no issue -> error message and return 0");
    freeAllTowers();
    setMemory(100000);
    sput_fail_unless(parse("while(tows<3)(mktwr int  b)")==1,"Valid -> return 1");
    setMemory(1000);
    sput_fail_unless(parse("while(mem>10)(mktwr int  b)")==1,"Valid -> return 1");
    freeAllTowers();
    setMemory(10000000);
     sput_fail_unless(parse("while ( tows < 3 ) ( mktwr int  b )")==1,"parse(""while ( tows < 3 ) ( mktwr int  b )"")==1. Valid, checks that spaces in the string dont mess up -> return 1");
    freeAllTowers();
    setMemory(1000);
    sput_fail_unless(parse("while(mem>=10)(mktwr int a)")==1,"Valid -> return 1");
    setMemory(1);
    createTowerFromPositions(1);
    sput_fail_unless(parse("while(mem>=10)(upgrade p t1)")==1,"Valid -> return 1");
}








