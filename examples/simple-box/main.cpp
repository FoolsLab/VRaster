#include <VRaster.hpp>

namespace Game {

glm::vec3 fwdVec, upperVec, rightVec;

ModelHandle cubeModel;
glm::vec3 cubePos;
glm::quat cubePose;

void init(IGraphicsProvider& g) {
    cubeModel = g.LoadModel("assets/testcube.glb");
}

void proc(const GameData& dat){
    if(dat.stagePose.has_value()){
		fwdVec = dat.stagePose->ori * glm::vec3(0, 0, -1);
		upperVec = dat.stagePose->ori * glm::vec3(0, 1, 0);
		rightVec = dat.stagePose->ori * glm::vec3(1, 0, 0);

        cubePos = dat.stagePose->pos + upperVec * 1.2f + fwdVec * 1.2f;
        cubePose = dat.stagePose->ori;
    }
}

void draw(IGraphicsProvider& g) {
    g.DrawModel(cubeModel, cubePos, cubePose, glm::vec3(0.1, 0.1, 0.1));
}

}
