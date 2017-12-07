
./protoc --cpp_out=../Classes \
funapi/management/maintenance_message.proto \
funapi/network/fun_message.proto \
funapi/network/ping_message.proto \
funapi/service/multicast_message.proto \
funapi/service/redirect_message.proto \
funapi/distribution/fun_dedicated_server_rpc_message.proto \
test_dedicated_server_rpc_messages.proto \
test_messages.proto
