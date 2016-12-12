
protoc --cpp_out=../Classes \
funapi/management/maintenance_message.proto \
funapi/network/fun_message.proto \
funapi/network/ping_message.proto \
funapi/service/multicast_message.proto \
funapi/service/redirect_message.proto \
test_messages.proto
