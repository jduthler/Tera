//---------------------------------------------------------------------------------------------------------------------------------------------------
// Tera
//
// Desc:	This application is a test mule used to develop and validate the radioddity GD-77 file layout.
//			Due to the test nature of this app it is brute force and has very few safety nets.
//
//			Radio firmware 3.0.6 was used in the development of parsers
//
//---------------------------------------------------------------------------------------------------------------------------------------------------
#include "stdafx.h"

#include "malloc.h"
#include <string.h>
#include <process.h>
#include <stdio.h>



using namespace std;

int		DumpChannels(FILE *CPSFile);
int		DumpContacts(FILE *CPSFile);
int		DumpZone(FILE *CPSFile);
int		DumpRXGroups(FILE *CPSFile);
int		DumpModelName(FILE *CPSFile);
int		DumpScanList(FILE *CPSFile);

void	Usage();

inline int BCDToString(unsigned char *sInput, char *sOutput);
inline int WordToString(unsigned char *sInput, char *sOutput);
inline int CallIDToString(unsigned char *sInput, char *sOutput);

FILE	*CPSFile = NULL;

//BYTE	ChannelNameList[CHANNEL_NAME_SLOTS][CHANNEL_NAME_SIZE];



int main(int argc, char *argv[], char *envp[])
{
	errno_t errorCode;
	char	czFileName[FILENAME_MAX];
	BYTE	CmdLineOption = 'a';			// default is dump channels if only the filename is provided
	BYTE	nNumFlag = 0;
	BYTE	FlagsRemain = 1;

	if (argc > 1)
	{
		strcpy_s(czFileName, argv[1]);
		printf("Dumping %s\r\n", czFileName);

		if ((errorCode = fopen_s(&CPSFile, czFileName, "rb")) != NULL)
		{
			printf("Unable to open file %s, errorCode %d\r\n", czFileName, errorCode);
			fclose(CPSFile);
			return 0;
		}
		else
		{
			if (!DumpModelName(CPSFile))
			{
				fclose(CPSFile);
				return 0;
			}
		}

		if (argc > 2)
		{
			nNumFlag = 1;
			FlagsRemain = argc - 2;
		}

		do
		{
			if (nNumFlag)
			{
				if (*argv[nNumFlag + 1] == '-')
					CmdLineOption = *(argv[nNumFlag + 1] + 1);
				++nNumFlag;
			}

			switch (CmdLineOption)
			{
			case	'c':
			case	'C':
				DumpContacts(CPSFile);
				break;
			case	'z':
			case	'Z':
				DumpZone(CPSFile);
				break;
			case	'a':
			case	'A':
				DumpChannels(CPSFile);
				break;
			case	'g':
			case	'G':
				DumpRXGroups(CPSFile);
				break;
			case	's':
			case	'S':
				DumpScanList(CPSFile);
				break;
			default:
				Usage();
				break;
			}

		} while (--FlagsRemain);
		fclose(CPSFile);
	}
	else
		Usage();

	return 0;
}
//---------------------------------------------------------------------------------------------------------------------------------------------------
// voide Usage()
//
// Desc: Make the user aware of the options. 
//
//---------------------------------------------------------------------------------------------------------------------------------------------------
void Usage()
{
	printf("\tUsage Tera [filename] <options>\r\n");
	printf("\r\n");
	printf("\t[filename] The mandatory file with extensio to open  \r\n");
	printf("\tOptions  \r\n");
	printf("\t-c Contacts\r\n");
	printf("\t-z Zones\r\n");
	printf("\t-a Channels(Default)\r\n");
	printf("\t-g RX Groups\r\n");
	printf("\t-s Scanlists\r\n");
}
//---------------------------------------------------------------------------------------------------------------------------------------------------
//  int DumpChannels(FILE *CPSFile)
//
//	Pass in sn openfile and the routine will dump the channels which of all of the data types is the most complex with the most amount of bit fields.
//  The real work was done in the development of the structure and unions which allow easy access to the stream contents.
//---------------------------------------------------------------------------------------------------------------------------------------------------
int DumpChannels(FILE *CPSFile)
{
	int	long	lChannelOffset = CHANNEL_DETAIL_START,
		lChannelDetailOffset,
		lChannelNameOffset,
		lChannel = 1;
	DBYTE		ChannelCount;
	static  char	czReadBuffer[CHANNEL_DETAIL_SIZE * 2],
					ChannelNameBuffer[CHANNEL_NAME_READSIZE],
					ChannelDetailBuffer[CHANNEL_DETAIL_READSIZE];

	char		ChannelName[CHANNEL_NAME_SIZE];
	char		czRXBuff[20];
	char		czTXBuff[20];
	char		szRXToneBuff[20];
	char		szTXToneBuff[20];
	char		szAdmit[20];
	size_t		ReadReturn;

	TeraChannel	*pTeraChannel;

	fseek(CPSFile, CHANNEL_COUNT_START, SEEK_SET);
	if (ReadReturn = fread(czReadBuffer, sizeof(char), 2, CPSFile))
	{ 
		ChannelCount = (DBYTE)czReadBuffer[0];
		printf("Dumping %u Channels\r\n\r\n", ChannelCount);
	}
	else
		return 0;
	
	fseek(CPSFile, CHANNEL_NAME_START, SEEK_SET);
	if (ReadReturn = fread(&ChannelNameBuffer[0], sizeof(char), CHANNEL_NAME_READSIZE, CPSFile)) // Read the max size
	{
		lChannelNameOffset = 0;
	}
	else
		return 0;

	fseek(CPSFile, CHANNEL_DETAIL_START, SEEK_SET);
	if (ReadReturn = fread(&ChannelDetailBuffer[0], sizeof(char), CHANNEL_DETAIL_READSIZE, CPSFile))	// Read the max size
	{
		lChannelDetailOffset = 0;
	}
	else
		return 0;


	do
	{

		pTeraChannel = (TeraChannel *)&ChannelDetailBuffer[lChannelDetailOffset];					// Point to the details
		memcpy(ChannelName, &ChannelNameBuffer[lChannelNameOffset], CHANNEL_NAME_SIZE);			// Extract out the name

		BCDToString(&pTeraChannel->m_szRXFreq[0], czRXBuff);
		BCDToString(&pTeraChannel->m_szTXFreq[0], czTXBuff);

		WordToString(&pTeraChannel->FuncBitsUnion.FuncBits.cbRXTone[0], szRXToneBuff);
		WordToString(&pTeraChannel->FuncBitsUnion.FuncBits.cbTXTone[0], szTXToneBuff);

		switch (pTeraChannel->FuncBitsUnion.FuncBits.AdmitMethod)
		{
		case 0:
			strcpy(szAdmit, "Always");
			break;
		case 1:
			strcpy(szAdmit, "Free");
			break;
		case 2:
			strcpy(szAdmit, "CCF");
			break;
		default:
			strcpy(szAdmit, "Error");
			break;
		}

		printf("Channel:[%0.5ld][%11.11s] RX:[%8.8s] TX:[%8.8s] RTone:[%4.4s] TTone[%4.4s] Talk Around:[%s] Admit:[%6.6s] Color:[%0.2u] Digital:[%s]\r\n\t\t\t\t\t\t\t\t\t\t\tRX Only:[%s], TS2:[%s] Wide:[%s]\r\n",
			lChannel,
			ChannelName,
			czRXBuff,
			czTXBuff,
			szRXToneBuff,
			szTXToneBuff,
			pTeraChannel->FuncBitsUnion.FuncBits.TalkAround ? "Y" : "N",
			szAdmit,
			pTeraChannel->FuncBitsUnion.FuncBits.ColorCode,
			pTeraChannel->FuncBitsUnion.FuncBits.Digital ? "Y" : "N",
			pTeraChannel->FuncBitsUnion.FuncBits.RXOnly ? "Y" : "N",
			pTeraChannel->FuncBitsUnion.FuncBits.UseTimeSlot2 ? "Y" : "N",
			pTeraChannel->FuncBitsUnion.FuncBits.AnalogWide ? "Y" : "N");


		lChannelDetailOffset += CHANNEL_DETAIL_SIZE;
		lChannelNameOffset += CHANNEL_NAME_SIZE;
		++lChannel;
		
	} while (ChannelCount--);

	return(1);
}
//---------------------------------------------------------------------------------------------------------------------------------------------------
// int DumpZone(FILE *CPSFile)
//
// Dump the contents of the zone section. Each zone can contain up to 16 Channels
//
//---------------------------------------------------------------------------------------------------------------------------------------------------
int DumpZone(FILE *CPSFile)
{
	int	long	lOffset = ZONE_START;
	char		czReadBuffer[ZONE_SIZE * 2];
	BYTE		bOffset = 0;
	size_t		ReadReturn;

	fseek(CPSFile, ZONE_START, SEEK_SET);
	TeraZone	*pTeraZone;

	do
	{
		if (ReadReturn = fread(czReadBuffer, sizeof(char), ZONE_SIZE, CPSFile))
		{
			if (ReadReturn != NULL)
				pTeraZone = (TeraZone *)&czReadBuffer[0];
			else
				return(0);

			if (*pTeraZone->m_szZoneName != 0)
			{
				bOffset = 0;
				printf("Zone  Name [%15.15s] ", pTeraZone->m_szZoneName);
				while (bOffset < ZONE_MAXCHANNELS)
				{
					if (*(pTeraZone->Channel + bOffset) != 0)
					{
						printf("%02d,", *(pTeraZone->Channel + bOffset));
						++bOffset;
					}
					else
						bOffset = ZONE_MAXCHANNELS;
				}
				printf("\r\n");
			}
		}
		else
			lOffset = ZONE_MAX;

		lOffset += ZONE_SIZE;
	} while (lOffset < ZONE_MAX);

	return(1);

}
//---------------------------------------------------------------------------------------------------------------------------------------------------
// int	DumpScanList(FILE *CPSFile)
//
// The scanlist contains a fair number of options affecting individual scan list behavior. And is weird in that the config file contains unused ScanList text
// entries on initialization. The lower nibble of a bit field byte is used to indicate valid entries. See struct def for details. 
//
//
//---------------------------------------------------------------------------------------------------------------------------------------------------
int	DumpScanList(FILE *CPSFile)
{
	int	long	lOffset = SCANLIST_START;
	char		czReadBuffer[SCANLIST_SIZE * 2];
	BYTE		bOffset = 0;
	size_t		ReadReturn;

	fseek(CPSFile, SCANLIST_START, SEEK_SET);
	TeraScanlist *pTeraScanList;

	do
	{
		if (ReadReturn = fread(czReadBuffer, sizeof(char), SCANLIST_SIZE, CPSFile))
		{
			pTeraScanList = (TeraScanlist *)&czReadBuffer[0];

			if (*pTeraScanList->m_szListName != 0xff && pTeraScanList->TeraScanBits.TeraScanBits.Valid == 0)
			{
				bOffset = 0;
				printf("ScanList Name [%15.15s] PLType:%u] Hold:[%u] SampleTime[%u] Channel1[%u] Channel2[%u]\r\n",
					pTeraScanList->m_szListName,
					pTeraScanList->TeraScanBits.TeraScanBits.PLType,
					pTeraScanList->HoldTime,
					pTeraScanList->SampleTime,
					pTeraScanList->PriorityChannel1,
					pTeraScanList->PriorityChannel2);
				printf("\t\t\t\tTalkback[%s] Channel Mark[%s]\r\n\t\t\t\t", pTeraScanList->TeraScanBits.TeraScanBits.TalkBack ? "Y" : "N",
					pTeraScanList->TeraScanBits.TeraScanBits.ChannelMark ? "Y" : "N");
				while (bOffset < SCANLIST_CHANNELLIMIT)
				{
					if (*(pTeraScanList->Member + bOffset) != 0)
					{
						printf("%02d,", *(pTeraScanList->Member + bOffset));
						++bOffset;
					}
					else
						bOffset = SCANLIST_CHANNELLIMIT;
				}
				printf("\r\n");
			}
			else
				lOffset = SCANLIST_MAX;
		}
		else
			lOffset = SCANLIST_MAX;

		lOffset += SCANLIST_SIZE;
	} while (lOffset < SCANLIST_MAX);

	return(1);

}//---------------------------------------------------------------------------------------------------------------------------------------------------
 //  int DumpRXGroups(FILE *CPSFile)
 //
 //
 //
 //---------------------------------------------------------------------------------------------------------------------------------------------------
int DumpRXGroups(FILE *CPSFile)
{
	int	long	lOffset = GROUPLIST_START;
	char		czReadBuffer[GROUPLIST_SIZE * 2];
	BYTE		bOffset = 0;
	size_t		ReadReturn;

	TeraGroupList *pTeraGroupList;

	fseek(CPSFile, GROUPLIST_START, SEEK_SET);

	do
	{
		if (ReadReturn = fread(czReadBuffer, sizeof(char), GROUPLIST_SIZE, CPSFile))
		{
			pTeraGroupList = (TeraGroupList *)&czReadBuffer[0];

			if (*pTeraGroupList->m_szGroupName != 0xff)
			{
				bOffset = 0;
				printf("RXGroupList Name [%15.15s] ", pTeraGroupList->m_szGroupName);
				while (bOffset < GROUPLIST_MAXCONTACTS)
				{
					if (*(pTeraGroupList->GroupList + bOffset) != 0)
					{
						printf("%02d,", *(pTeraGroupList->GroupList + bOffset));
						++bOffset;
					}
					else
						bOffset = GROUPLIST_MAXCONTACTS;
				}
				printf("\r\n");
			}
		}
		else
			lOffset = GROUPLIST_MAX;

		lOffset += GROUPLIST_SIZE;
	} while (lOffset < GROUPLIST_MAX);

	return(1);

}
//---------------------------------------------------------------------------------------------------------------------------------------------------
// int DumpContacts(FILE *CPSFile)
//
// Contacts are simple with only 5 elements not very efficently stored
//
//---------------------------------------------------------------------------------------------------------------------------------------------------
int DumpContacts(FILE *CPSFile)
{
	int	long	lOffset = CONTACT_START;
	char		czReadBuffer[CONTACT_SIZE * 2];
	char		czCallIDBuff[20];

	size_t		ReadReturn;
	TeraContact	*pTeraContact;

	fseek(CPSFile, CONTACT_START, SEEK_SET);

	do
	{
		if (ReadReturn = fread(czReadBuffer, sizeof(char), CONTACT_SIZE, CPSFile))
		{
			if (ReadReturn != NULL)
				pTeraContact = (TeraContact *)&czReadBuffer[0];
			if (*pTeraContact->m_szContactName != 0xff)
			{
				CallIDToString(pTeraContact->CallID, czCallIDBuff);

				printf("Contact[%15.15s] CallID:[%08.8s] CallType:[%d] RingStyle[%02d] Call Receive Tone[%s]\r\n",
					pTeraContact->m_szContactName, czCallIDBuff,
					pTeraContact->CallType,
					pTeraContact->RingStyle,
					pTeraContact->CallReceiveTone ? "Y" : "N");
			}
		}
		else
			lOffset = CONTACT_MAX;

		lOffset += CONTACT_SIZE;
	} while (lOffset < CONTACT_MAX);

	return(1);
}
//---------------------------------------------------------------------------------------------------------------------------------------------------
// int		DumpModelName(FILE *CPSFile)
//
// I was curious if I could find the firmware rev in the file as it may be nessasary to detect if the mapping changes over the revisions 
//
//---------------------------------------------------------------------------------------------------------------------------------------------------
int		DumpModelName(FILE *CPSFile)
{
	int	long	lSize;
	char		czReadBuffer[MODELNAME_SIZE * 2];
	size_t		ReadReturn;

	fseek(CPSFile, 0, SEEK_END);
	lSize = ftell(CPSFile);
	printf("File is %d bytes\n\r", lSize);
	rewind(CPSFile);
	fseek(CPSFile, MODELNAME_START, SEEK_SET);
	TeraModelName *pTeraModelName;


	if (ReadReturn = fread(czReadBuffer, sizeof(char), MODELNAME_SIZE, CPSFile))
	{
		pTeraModelName = (TeraModelName *)&czReadBuffer[0];
		printf("ModelName [%8.8s] Firmware:%02u.%02u.%02u \r\n", pTeraModelName->m_szModelName,
			pTeraModelName->FirmwareVersion[3],
			pTeraModelName->FirmwareVersion[2],
			pTeraModelName->FirmwareVersion[1]);

	}
	else
	{
		printf("Unable to read Model name and firmware version -- Exiting\r\n");
		return(0);
	}

	rewind(CPSFile);			// Other functions assume file pointer at the beginning
	return(1);

}
//---------------------------------------------------------------------------------------------------------------------------------------------------
// Misc conversion routines
//
//
//
//---------------------------------------------------------------------------------------------------------------------------------------------------
inline int BCDToString(unsigned char *sInput, char *sOutput)
{
	sprintf(sOutput, "%x%x%x%x", *(sInput + 3), *(sInput + 2), *(sInput + 1), *(sInput));
	return(1);
}
inline int CallIDToString(unsigned char *sInput, char *sOutput)
{
	sprintf(sOutput, "%x%x%x%x", *(sInput), *(sInput + 1), *(sInput + 2), *(sInput + 3));
	return(1);
}
inline int WordToString(unsigned char *sInput, char *sOutput)
{
	if (*sInput == 0xff && *(sInput + 1) == 0xff)
		sprintf(sOutput, "   ");
	else
		sprintf(sOutput, "%x%x", *(sInput + 1), *(sInput));
	return(1);
}


