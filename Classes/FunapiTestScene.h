#ifndef __FUNAPI_TEST_SCENE_H__
#define __FUNAPI_TEST_SCENE_H__

#include "cocos2d.h"

class FunapiTest : public cocos2d::Layer
{
public:
  static cocos2d::Scene* createScene();
  
  virtual bool init();

  // implement the "static create()" method manually
  CREATE_FUNC(FunapiTest);
  
private:
  const std::string kServerIp = "jhp-vmware";
};

#endif // __FUNAPI_TEST_SCENE_H__
