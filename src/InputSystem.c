//
//  InputSystem.c
//  Blob Game
//
//  Created by Stefan Johnson on 3/07/2015.
//  Copyright (c) 2015 Stefan Johnson. All rights reserved.
//

#include "InputSystem.h"
#include "Window.h"
#include "Keyboard.h"
#include "InputMapKeyboardComponent.h"
#include "InputMapGroupComponent.h"

typedef struct {
    double timestamp;
    CCInputState active;
} CCInputMapGroupState;

static void CCWindowFocus(GLFWwindow *Window, int Focus);
static _Bool CCInputSystemHandlesComponent(CCComponentID id);
static void CCInputSystemUpdate(void *Context, CCCollection Components);
static CCInputMapGroupState CCInputSystemGetGroupStateForComponent(CCComponent Component);
static CCVector2D CCInputSystemGetSimulatedGroupPressure2(CCComponent Component);
static CCVector3D CCInputSystemGetSimulatedGroupPressure3(CCComponent Component);

void CCInputSystemRegister(void)
{
    CCKeyboardStateReset();
    
    glfwSetWindowFocusCallback(CCWindow, CCWindowFocus);
    
    glfwSetKeyCallback(CCWindow, CCKeyboardInput);
    glfwSetCharModsCallback(CCWindow, CCKeyboardCharInput);
    
    CCComponentSystemRegister(CC_INPUT_SYSTEM_ID, CCComponentSystemExecutionTypeInput, CCInputSystemUpdate, CCInputSystemHandlesComponent, NULL, NULL, NULL, NULL, NULL);
}

void CCInputSystemDeregister(void)
{
    CCComponentSystemDeregister(CC_INPUT_SYSTEM_ID, CCComponentSystemExecutionTypeInput);
}

static CCCollection CCInputSystemGetComponentsInCollection(CCCollection Group, CCInputMapType InputType)
{
    CCAssertLog(InputType != CCInputMapTypeNone, "Should not try find components for this type");
    _Static_assert(CCInputMapTypeKeyboard == 2 &&
                   CCInputMapTypeMouse == 3 &&
                   CCInputMapTypeController == 4 &&
                   CCInputMapTypeGroup == 5, "Expects input types to have these values");
    
    CCComponentID Type = (CCComponentID[]){ CC_INPUT_MAP_KEYBOARD_COMPONENT_ID, 0, 0, CC_INPUT_MAP_GROUP_COMPONENT_ID }[InputType - CCInputMapTypeKeyboard];
    
    CCEnumerator Enumerator;
    CCCollectionGetEnumerator(Group, &Enumerator);
    
    CCCollection InputComponents = CCCollectionCreate(CC_STD_ALLOCATOR, CCCollectionHintHeavyEnumerating | CCCollectionHintHeavyInserting, sizeof(CCComponent), NULL);
    for (CCComponent *Component = CCCollectionEnumeratorGetCurrent(&Enumerator); Component; Component = CCCollectionEnumeratorNext(&Enumerator))
    {
        CCComponentID id = CCComponentGetID(*Component);
        if (id == Type)
        {
            CCCollectionInsertElement(InputComponents, Component);
        }
        
        else if (id == CCInputMapTypeGroup) //TODO: Should we retrieve nested groups if asking for groups? (e.g. remove the 'else')
        {
            CCCollection Children = CCInputSystemGetComponentsInCollection(CCInputMapGroupComponentGetInputMaps(*Component), InputType);
            CCCollectionInsertCollection(InputComponents, Children, NULL); //TODO: consume insert
            CCCollectionDestroy(Children);
        }
    }
    
    return InputComponents;
}

CCCollection CCInputSystemGetComponents(CCInputMapType InputType)
{
    return CCInputSystemGetComponentsInCollection(CCComponentSystemGetComponentsForSystem(CC_INPUT_SYSTEM_ID), InputType);
}

static CCComponent CCInputSystemFindComponentForActionInCollection(CCCollection Group, const char *Action)
{
    CCEnumerator Enumerator;
    CCCollectionGetEnumerator(Group, &Enumerator);
    
    for (CCComponent *Component = CCCollectionEnumeratorGetCurrent(&Enumerator); Component; Component = CCCollectionEnumeratorNext(&Enumerator))
    {
        CCComponentID id = CCComponentGetID(*Component);
        if ((id & CC_INPUT_COMPONENT_FLAG) == CC_INPUT_COMPONENT_FLAG)
        {
            const char *InputAction = CCInputMapComponentGetAction(*Component);
            if ((InputAction) && (!strcmp(InputAction, Action))) return *Component;
            else if (id == CC_INPUT_MAP_GROUP_COMPONENT_ID)
            {
                CCComponent Input = CCInputSystemFindComponentForActionInCollection(CCInputMapGroupComponentGetInputMaps(*Component), Action);
                if (Input) return Input;
            }
        }
    }
    
    return NULL;
}

CCInputState CCInputSystemGetStateForAction(CCEntity Entity, const char *Action)
{
    CCComponent Input = CCInputSystemFindComponentForActionInCollection(CCEntityGetComponents(Entity), Action);
    if (Input)
    {
        switch (CCComponentGetID(Input))
        {
            case CC_INPUT_MAP_KEYBOARD_COMPONENT_ID:;
                CCKeyboardState KeyState = CCKeyboardGetStateForComponent(Input);
                return KeyState.down ? CCInputStateActive : CCInputStateInactive;
                
            case CC_INPUT_MAP_GROUP_COMPONENT_ID:;
                CCInputMapGroupState GroupState = CCInputSystemGetGroupStateForComponent(Input);
                return GroupState.active;
                
            default:
                break;
        }
    }
    
    return CCInputStateInactive;
}

static float CCInputSystemPressureForBinaryInput(CCInputState State, double Timestamp, float Ramp)
{
    Ramp *= (glfwGetTime() - Timestamp);
    Ramp = Ramp == 0.0f ? 1.0f : Ramp;
    return CCClampf((State == CCInputStateActive ? 0.0f + Ramp : 1.0f - Ramp), 0.0f, 1.0f);
}

float CCInputSystemGetPressureForAction(CCEntity Entity, const char *Action)
{
    CCComponent Input = CCInputSystemFindComponentForActionInCollection(CCEntityGetComponents(Entity), Action);
    if (Input)
    {
        switch (CCComponentGetID(Input))
        {
            case CC_INPUT_MAP_KEYBOARD_COMPONENT_ID:;
                CCKeyboardState KeyState = CCKeyboardGetStateForComponent(Input);
                return CCInputSystemPressureForBinaryInput(KeyState.down ? CCInputStateActive : CCInputStateInactive, KeyState.timestamp, CCInputMapKeyboardComponentGetRamp(Input));
                
            case CC_INPUT_MAP_GROUP_COMPONENT_ID:;
                CCInputMapGroupState GroupState = CCInputSystemGetGroupStateForComponent(Input);
                return CCInputSystemPressureForBinaryInput(GroupState.active, GroupState.timestamp, 0.0f); //TODO: would we want groups to have value ramps?
                
            default:
                break;
        }
    }
    
    return 0.0f;
}

CCVector2D CCInputSystemGetPressure2ForAction(CCEntity Entity, const char *Action)
{
    CCComponent Input = CCInputSystemFindComponentForActionInCollection(CCEntityGetComponents(Entity), Action);
    if (Input)
    {
        switch (CCComponentGetID(Input))
        {
            case CC_INPUT_MAP_GROUP_COMPONENT_ID:
                return CCInputSystemGetSimulatedGroupPressure2(Input);
                
            default:
                break;
        }
    }
    
    return CCVector2DZero;
}

CCVector3D CCInputSystemGetPressure3ForAction(CCEntity Entity, const char *Action)
{
    CCComponent Input = CCInputSystemFindComponentForActionInCollection(CCEntityGetComponents(Entity), Action);
    if (Input)
    {
        switch (CCComponentGetID(Input))
        {
            case CC_INPUT_MAP_GROUP_COMPONENT_ID:
                return CCInputSystemGetSimulatedGroupPressure3(Input);
                
            default:
                break;
        }
    }
    
    return CCVector3DZero;
}

static void CCWindowFocus(GLFWwindow *Window, int Focus)
{
    if (!Focus)
    {
        CCKeyboardStateReset();
    }
}

static _Bool CCInputSystemHandlesComponent(CCComponentID id)
{
    return (id & 0x7f000000) == CC_INPUT_COMPONENT_FLAG;
}

static void CCInputSystemUpdate(void *Context, CCCollection Components)
{
    CCCollectionDestroy(CCComponentSystemGetAddedComponentsForSystem(CC_INPUT_SYSTEM_ID));
    CCCollectionDestroy(CCComponentSystemGetRemovedComponentsForSystem(CC_INPUT_SYSTEM_ID));
}

static CCInputMapGroupState CCInputSystemGetGroupStateForComponent(CCComponent Component)
{
    CCEnumerator Enumerator;
    CCCollectionGetEnumerator(CCInputMapGroupComponentGetInputMaps(Component), &Enumerator);
    
    CCInputState AllActive = CCInputStateActive, OneActive = CCInputStateInactive;
    double LatestActiveTimestamp = 0.0, LatestInactiveTimestamp = 0.0;
    for (CCComponent *Input = CCCollectionEnumeratorGetCurrent(&Enumerator); Input; Input = CCCollectionEnumeratorNext(&Enumerator))
    {
        switch (CCComponentGetID(*Input))
        {
            case CC_INPUT_MAP_KEYBOARD_COMPONENT_ID:;
                const CCKeyboardState KeyState = CCKeyboardGetStateForComponent(*Input);
                if (KeyState.down)
                {
                    OneActive = CCInputStateActive;
                    if (LatestActiveTimestamp < KeyState.timestamp) LatestActiveTimestamp = KeyState.timestamp;
                }
                
                else
                {
                    AllActive = CCInputStateInactive;
                    if (LatestInactiveTimestamp < KeyState.timestamp) LatestInactiveTimestamp = KeyState.timestamp;
                }
                break;
                
            case CC_INPUT_MAP_GROUP_COMPONENT_ID:;
                const CCInputMapGroupState ChildState = CCInputSystemGetGroupStateForComponent(*Input);
                if (ChildState.active)
                {
                    OneActive = CCInputStateActive;
                    if (LatestActiveTimestamp < ChildState.timestamp) LatestActiveTimestamp = ChildState.timestamp;
                }
                
                else
                {
                    AllActive = CCInputStateInactive;
                    if (LatestInactiveTimestamp < ChildState.timestamp) LatestInactiveTimestamp = ChildState.timestamp;
                }
                break;
                
            default:
                break;
        }
    }
    
    const CCInputState Active = CCInputMapGroupComponentGetWantsAllActive(Component) ? AllActive : OneActive;
    return (CCInputMapGroupState){ .timestamp = Active ? LatestActiveTimestamp : LatestInactiveTimestamp, .active = Active };
}

static CCVector2D CCInputSystemGetSimulatedGroupPressure2(CCComponent Component)
{
    CCOrderedCollection Inputs = CCInputMapGroupComponentGetInputMaps(Component);
    const size_t Count = CCCollectionGetCount(Inputs);
    
    CCAssertLog(Count == 2 || Count == 4, "To correctly simulate a 2 axis input device, there must either be 2 or 4 single axis inputs");
    
    CCEnumerator Enumerator;
    CCCollectionGetEnumerator(Inputs, &Enumerator);
    
    float Pressure[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    for (CCComponent *Input = CCCollectionEnumeratorGetCurrent(&Enumerator); Input; Input = CCCollectionEnumeratorNext(&Enumerator))
    {
        const size_t Index = CCOrderedCollectionGetIndex(Inputs, CCCollectionEnumeratorGetEntry(&Enumerator));
        switch (CCComponentGetID(*Input))
        {
            case CC_INPUT_MAP_KEYBOARD_COMPONENT_ID:;
                CCKeyboardState KeyState = CCKeyboardGetStateForComponent(*Input);
                Pressure[Index] = CCInputSystemPressureForBinaryInput(KeyState.down ? CCInputStateActive : CCInputStateInactive, KeyState.timestamp, CCInputMapKeyboardComponentGetRamp(*Input));
                break;
                
            default:
                CCAssertLog(0, "Must contain only single axis inputs");
                break;
        }
    }
    
    return Count == 2 ? CCVector2DMake(Pressure[0], Pressure[1]) : CCVector2DMake(Pressure[2] - Pressure[0], Pressure[3] - Pressure[1]);
}

static CCVector3D CCInputSystemGetSimulatedGroupPressure3(CCComponent Component)
{
    CCOrderedCollection Inputs = CCInputMapGroupComponentGetInputMaps(Component);
    const size_t Count = CCCollectionGetCount(Inputs);
    
    CCAssertLog(Count == 3 || Count == 6, "To correctly simulate a 3 axis input device, there must either be 3 or 6 single axis inputs");
    
    CCEnumerator Enumerator;
    CCCollectionGetEnumerator(Inputs, &Enumerator);
    
    float Pressure[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    for (CCComponent *Input = CCCollectionEnumeratorGetCurrent(&Enumerator); Input; Input = CCCollectionEnumeratorNext(&Enumerator))
    {
        const size_t Index = CCOrderedCollectionGetIndex(Inputs, CCCollectionEnumeratorGetEntry(&Enumerator));
        switch (CCComponentGetID(*Input))
        {
            case CC_INPUT_MAP_KEYBOARD_COMPONENT_ID:;
                CCKeyboardState KeyState = CCKeyboardGetStateForComponent(*Input);
                Pressure[Index] = CCInputSystemPressureForBinaryInput(KeyState.down ? CCInputStateActive : CCInputStateInactive, KeyState.timestamp, CCInputMapKeyboardComponentGetRamp(*Input));
                break;
                
            default:
                CCAssertLog(0, "Must contain only single axis inputs");
                break;
        }
    }
    
    return Count == 3 ? CCVector3DMake(Pressure[0], Pressure[1], Pressure[2]) : CCVector3DMake(Pressure[3] - Pressure[0], Pressure[4] - Pressure[1], Pressure[5] - Pressure[2]);
}
