#include "../callater.h"
#include <stdio.h>
#include <stdbool.h>

static int test_counter = 0;
static int assert_counter = 0;
static int success_counter = 0;
float mock_current_time = 0.0f;

#define TEST(name) printf("Test %d: %s\n", ++test_counter, name)
#define ASSERT(cond) do { \
assert_counter++; \
if (!(cond)) { \
    printf("FAIL (Line %d)\n", __LINE__); \
    return; \
} \
printf("PASS\n"); \
success_counter++; \
} while(0)

#define CALLATER_TEST
#include "../callater.c"

void setup();

// Test callbacks
static int basic_callback_count = 0;
static int repeat_callback_count = 0;
static int group_callback_count = 0;
static int multi_callback_count = 0;

void BasicCallback(void* arg, CallaterRef ref) {
    basic_callback_count++;
}

void RepeatCallback(void* arg, CallaterRef ref) {
    repeat_callback_count++;
}

void GroupCallback(void* arg, CallaterRef ref) {
    group_callback_count++;
}

void MultiCallback(void* arg, CallaterRef ref) {
    multi_callback_count++;
}

// Test cases
void TestBasicInvocation() {
    TEST("Basic invocation");
    setup();
    
    CallaterInvoke(BasicCallback, NULL, 1.0f);
    
    // Before delay
    mock_current_time = 0.5f;
    CallaterUpdate();
    ASSERT(basic_callback_count == 0);
    
    // After delay
    mock_current_time = 1.5f;
    CallaterUpdate();
    ASSERT(basic_callback_count == 1);
}

void TestRepeatInvocation() {
    TEST("Repeated invocation");
    setup();
    
    CallaterRef ref = CallaterInvokeRepeat(RepeatCallback, NULL, 0.5f, 1.0f);
    
    // First invocation
    mock_current_time = 0.6f;
    CallaterUpdate();
    ASSERT(repeat_callback_count == 1);
    
    // Second invocation
    mock_current_time = 1.6f;
    CallaterUpdate();
    ASSERT(repeat_callback_count == 2);
    
    // Cancel repeat
    CallaterCancel(ref);
    mock_current_time = 2.6f;
    CallaterUpdate();
    ASSERT(repeat_callback_count == 2);
}

void TestGroupCancellation() {
    TEST("Group cancellation");
    setup();
    
    const uint64_t GROUP_ID = 42;
    CallaterInvokeGID(GroupCallback, NULL, 0.1f, GROUP_ID);
    CallaterInvokeGID(GroupCallback, NULL, 0.2f, GROUP_ID);
    CallaterInvokeGID(GroupCallback, NULL, 0.3f, GROUP_ID);
    
    CallaterCancelGID(GROUP_ID);
    
    mock_current_time = 1.0f;
    CallaterUpdate();
    ASSERT(group_callback_count == 0);
}

void TestFunctionCancellation() {
    TEST("Function cancellation");
    setup();
    
    CallaterInvoke(MultiCallback, NULL, 0.1f);
    CallaterInvoke(MultiCallback, NULL, 0.2f);
    CallaterInvoke(MultiCallback, NULL, 0.3f);
    
    CallaterCancelFunc(MultiCallback);
    
    mock_current_time = 1.0f;
    CallaterUpdate();
    ASSERT(multi_callback_count == 0);
}

void TestImmediateInvocation() {
    TEST("Immediate invocation");
    setup();
    
    CallaterInvoke(BasicCallback, NULL, 0.0f);
    
    mock_current_time = 0.0f;
    CallaterUpdate();
    ASSERT(basic_callback_count == 1);
}

void TestReferenceManagement() {
    TEST("Reference management");
    setup();
    
    CallaterRef ref = CallaterInvoke(BasicCallback, NULL, 0.5f);
    ASSERT(CallaterFuncRef(BasicCallback).ref == ref.ref);
    
    CallaterCancel(ref);
    mock_current_time = 1.0f;
    CallaterUpdate();
    ASSERT(basic_callback_count == 0);
}

void TestStressTest() {
    TEST("Stress test");
    setup();
    
    const int NUM_CALLBACKS = 1000;
    for (int i = 1; i <= NUM_CALLBACKS; i++)
    {
        CallaterInvoke(MultiCallback, NULL, (i * 10.0f) / NUM_CALLBACKS);
    }
    
    mock_current_time = 5;
    CallaterUpdate();
    ASSERT(multi_callback_count == NUM_CALLBACKS / 2);
    
    mock_current_time = 10;
    CallaterUpdate();
    ASSERT(multi_callback_count == NUM_CALLBACKS);
}

void TestRepeatInvocationWithGID() {
    TEST("Repeated invocation with ID");
    setup();
    
    const uint64_t GROUP_ID = 123;
    CallaterRef ref = CallaterInvokeRepeatGID(RepeatCallback, NULL, 0.5f, 1.0f, GROUP_ID);
    
    // First invocation
    mock_current_time = 0.6f;
    CallaterUpdate();
    ASSERT(repeat_callback_count == 1);
    
    // Second invocation
    mock_current_time = 1.6f;
    CallaterUpdate();
    ASSERT(repeat_callback_count == 2);
    
    // Check group ID
    ASSERT(CallaterGetGID(ref) == GROUP_ID);
}

void TestSetFunction() {
    TEST("Set function");
    setup();
    
    CallaterRef ref = CallaterInvoke(BasicCallback, NULL, 0.5f);
    CallaterSetFunc(ref, RepeatCallback);
    
    mock_current_time = 0.6f;
    CallaterUpdate();
    ASSERT(basic_callback_count == 0);
    ASSERT(repeat_callback_count == 1);
}

void TestSetRepeatRate() {
    TEST("Set repeat rate");
    setup();
    
    CallaterRef ref = CallaterInvoke(RepeatCallback, NULL, 0.5f);
    CallaterSetRepeatRate(ref, 1.0f);
    
    // First invocation
    mock_current_time = 0.6f;
    CallaterUpdate();
    ASSERT(repeat_callback_count == 1);
    
    // Second invocation
    mock_current_time = 1.4f;
    CallaterUpdate();
    ASSERT(repeat_callback_count == 1);
    
    // Third invocation
    mock_current_time = 1.6f;
    CallaterUpdate();
    ASSERT(repeat_callback_count == 2);
}

void TestSetGID() {
    TEST("Set ID");
    setup();
    
    CallaterRef ref = CallaterInvoke(BasicCallback, NULL, 0.5f);
    const uint64_t GROUP_ID = 456;
    CallaterSetGID(ref, GROUP_ID);
    
    ASSERT(CallaterGetGID(ref) == GROUP_ID);
}

void TestGetRepeatRate() {
    TEST("Get repeat rate");
    setup();
    
    CallaterRef ref = CallaterInvokeRepeat(RepeatCallback, NULL, 0.5f, 1.0f);
    ASSERT(CallaterGetRepeatRate(ref) == 1.0f);
    
    CallaterSetRepeatRate(ref, 2.0f);
    ASSERT(CallaterGetRepeatRate(ref) == 2.0f);
}

void TestGroupCount() {
    TEST("Group count");
    setup();
    
    const uint64_t GROUP_ID = 789;
    CallaterInvokeGID(BasicCallback, NULL, 0.1f, GROUP_ID);
    CallaterInvokeGID(BasicCallback, NULL, 0.2f, GROUP_ID);
    CallaterInvokeGID(BasicCallback, NULL, 0.3f, GROUP_ID);
    
    ASSERT(CallaterGroupCount(GROUP_ID) == 3);
}

bool RefsContain(CallaterRef *refs, uint64_t len, CallaterRef toFind)
{
    for(uint64_t i = 0 ; i < len ; i++)
    {
        if(refs[i].ref == toFind.ref) return true;
    }
    return false;
}

void TestGetGroupRefs() {
    TEST("Get group refs");
    setup();
    
    const uint64_t GROUP_ID = 101;
    CallaterRef refs[3];
    CallaterRef r1 = CallaterInvokeGID(BasicCallback, NULL, 0.1f, GROUP_ID);
    CallaterRef r2 = CallaterInvokeGID(BasicCallback, NULL, 0.2f, GROUP_ID);
    CallaterRef r3 = CallaterInvokeGID(BasicCallback, NULL, 0.3f, GROUP_ID);
    
    uint64_t count = CallaterGetGroupRefs(refs, GROUP_ID);
    ASSERT(RefsContain(refs, count, r1) && RefsContain(refs, count, r2) && RefsContain(refs, count, r3));
    ASSERT(count == 3);
}

void ModifyRepeatRateCallback(void* arg, CallaterRef ref) {
    repeat_callback_count++;
    if (repeat_callback_count == 1) {
        // Change the repeat rate during the first invocation
        CallaterSetRepeatRate(ref, 0.5f);
    }
}

void TestModifyRepeatRateDuringCallback() {
    TEST("Modify repeat rate during callback");
    setup();
    
    CallaterInvokeRepeat(ModifyRepeatRateCallback, NULL, 0.5f, 1.0f);
    
    // First invocation
    mock_current_time = 0.6f;
    CallaterUpdate();
    ASSERT(repeat_callback_count == 1);
    
    // Second invocation (should happen sooner due to modified repeat rate)
    mock_current_time = 1.1f;
    CallaterUpdate();
    ASSERT(repeat_callback_count == 2);
    
    // Third invocation
    mock_current_time = 1.6f;
    CallaterUpdate();
    ASSERT(repeat_callback_count == 3);
}

void CancelDuringCallback(void* arg, CallaterRef ref) {
    basic_callback_count++;
    // Cancel the invocation during the callback
    CallaterCancel(ref);
}

void TestCancelDuringCallback() {
    TEST("Cancel during callback");
    setup();
    
    CallaterInvoke(CancelDuringCallback, NULL, 0.5f);
    
    // First invocation
    mock_current_time = 0.6f;
    CallaterUpdate();
    ASSERT(basic_callback_count == 1);
    
    // Second update (should not invoke again)
    mock_current_time = 1.1f;
    CallaterUpdate();
    ASSERT(basic_callback_count == 1);
}

void ChangeGroupIdCallback(void* arg, CallaterRef ref) {
    group_callback_count++;
    // Change the group ID during the callback
    CallaterSetGID(ref, *(uint64_t*)arg);
}

void TestChangeGroupIdDuringCallback()
{
    TEST("Change group ID during callback");
    setup();
    
    uint64_t NEW_GROUP_ID = 999;
    
    CallaterRef ref = InvokeRepeatGID(ChangeGroupIdCallback, &NEW_GROUP_ID, 0.5f, 0.5f, 6969);
    
    // First invocation
    mock_current_time = 0.6f;
    CallaterUpdate();
    ASSERT(group_callback_count == 1);
    ASSERT(CallaterGetGID(ref) == NEW_GROUP_ID);
}

void NewFunctionCallback(void* arg, CallaterRef ref) {
    multi_callback_count++;
}

void SetNewFunctionCallback(void* arg, CallaterRef ref) {
    basic_callback_count++;
    // Set a new function during the callback
    CallaterSetFunc(ref, NewFunctionCallback);
    CallaterSetRepeatRate(ref, 0.5f);
}

void TestSetNewFunctionDuringCallback() {
    TEST("Set new function during callback");
    setup();
    
    CallaterRef ref = CallaterInvoke(SetNewFunctionCallback, NULL, 0.5f);
    
    // First invocation
    mock_current_time = 0.6f;
    CallaterUpdate();
    ASSERT(basic_callback_count == 1);
    ASSERT(multi_callback_count == 0);
    
    // Second invocation (should call the new function)
    mock_current_time = 1.1f;
    CallaterUpdate();
    ASSERT(multi_callback_count == 1);
}

void StopRepeatCallback(void* arg, CallaterRef ref) {
    repeat_callback_count++;
    if (repeat_callback_count == 1) {
        // Stop the repeat during the first invocation
        CallaterStopRepeat(ref);
    }
}

void TestStopRepeatDuringCallback() {
    TEST("Stop repeat during callback");
    setup();
    
    CallaterInvokeRepeat(StopRepeatCallback, NULL, 0.5f, 1.0f);
    
    // First invocation
    mock_current_time = 0.6f;
    CallaterUpdate();
    ASSERT(repeat_callback_count == 1);
    
    // Second update (should not invoke again)
    mock_current_time = 1.6f;
    CallaterUpdate();
    ASSERT(repeat_callback_count == 1);
}

int main() {
    printf("Starting Callater tests\n");
    
    TestBasicInvocation();
    TestRepeatInvocation();
    TestGroupCancellation();
    TestFunctionCancellation();
    TestImmediateInvocation();
    TestReferenceManagement();
    TestStressTest();
    
    TestRepeatInvocationWithGID();
    TestSetFunction();
    TestSetRepeatRate();
    TestSetGID();
    TestGetRepeatRate();
    TestGroupCount();
    TestGetGroupRefs();
    
    TestModifyRepeatRateDuringCallback();
    TestCancelDuringCallback();
    TestChangeGroupIdDuringCallback();
    TestSetNewFunctionDuringCallback();
    TestStopRepeatDuringCallback();
    
    printf("\nTest results: %d/%d passed\n", success_counter, assert_counter);
    return success_counter == assert_counter ? 0 : 1;
}

// Test setup/teardown
void setup()
{
    CallaterDeinit();
    CallaterInit();
    basic_callback_count = 0;
    repeat_callback_count = 0;
    group_callback_count = 0;
    multi_callback_count = 0;
    mock_current_time = 0.0f;
}
