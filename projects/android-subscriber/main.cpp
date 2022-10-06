#include <iostream>
#include <dds/dds.h>

int main(int argc, char *argv[])
{
    dds_entity_t dp = dds_create_participant(DDS_DOMAIN_DEFAULT, NULL, NULL);
    std::cout << "Domain participant: " << dp << std::endl;
    return 0;
}