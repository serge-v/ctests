#include <Carbon/Carbon.h>
#include <MacWindows.h>

int main(int ArgC, char **ArgV)
{
	Rect rPos;
	WindowRef Handle;
	EventTypeSpec etsEventTypes[2];

	rPos.top = 100;
	rPos.left = 100;
	rPos.bottom = 400;
	rPos.right = 500;

	//Create the window
	OSStatus err = CreateNewWindow(kPlainWindowClass, kWindowStandardDocumentAttributes, &rPos, &Handle);

	printf("Handle: %p, err: %d\n", Handle, err);
	//Show the window
	ShowWindow(Handle);

	EventRecord Event;
	while (1) {
		WaitNextEvent(everyEvent, &Event, (unsigned int) - 1, NULL);
		printf("Got an event!\n");
	}

	DisposeWindow(Handle);

	return 0;
}
