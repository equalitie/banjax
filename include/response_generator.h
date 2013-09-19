/*
*  Part of regex_ban plugin: based on the status of the request generates
*  approperiate respones
*
*  Vmon: May 2013: Initial version.
*/
#ifndef RESPONSE_GENERATOR_H
#define RESPONSE_GENERATOR_H
class ResponseGenerator
{
 public:
  /*
    generate the approperiate response when an ip is believe to engage in
    a ddos attack

    @return: pointer to the buffer that contains the response
  */
  char* respond_ddos();

};
  


#endif /* response_generator.h */


