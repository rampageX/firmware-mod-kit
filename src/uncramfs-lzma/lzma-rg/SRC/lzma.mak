LOCAL_OBJS=$(OBJS:%.o=local_%.o)

OTHER_TASKS=$(LOCAL_OBJS)
CLEAN=$(LOCAL_OBJS)

LOCAL_CFLAGS+=-D_NO_EXCEPTIONS

include $(RGMK)

$(LOCAL_OBJS) : local_%.o : %.cpp
	$(CXX_FOR_BUILD) $(LOCAL_CFLAGS) $(CFLAGS_$@) -I$(RGSRC)/pkg/include -c -o $@ $<
