/*
 * Copyright 2011-2012 Arx Libertatis Team (see the AUTHORS file)
 *
 * This file is part of Arx Libertatis.
 *
 * Arx Libertatis is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Arx Libertatis is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arx Libertatis.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "LegacyMathTest.h"

#include "graphics/Math.h"

#include "AssertionTraits.h"
#include "LegacyMath.h"

CPPUNIT_TEST_SUITE_REGISTRATION(LegacyMathTest);

struct TestRotation {
	glm::quat quat;
	Anglef angle;
	
	TestRotation(glm::quat quat, Anglef angle)
		: quat(quat)
		, angle(angle)
	{}
};

std::vector<TestRotation> rotations;

static void addTestData(glm::quat quat, Anglef angle) {
	rotations.push_back(TestRotation(quat, angle));
}

void LegacyMathTest::setUp() {
	// Data from: http://www.euclideanspace.com/maths/geometry/rotations/conversions/eulerToQuaternion/steps/index.htm
	
	// Identity (no rotation)
	addTestData(glm::quat(    1.f,    0.0f,    0.0f,    0.0f), Anglef(  0.f,   0.f,   0.f));
	// 90 degrees about y axis
	addTestData(glm::quat(0.7071f,    0.0f, 0.7071f,    0.0f), Anglef( 90.f,   0.f,   0.f));
	// 180 degrees about y axis
	addTestData(glm::quat(   0.0f,    0.0f,     1.f,    0.0f), Anglef(180.f,   0.f,   0.f));
	// 270 degrees about y axis
	addTestData(glm::quat(0.7071f,    0.0f,-0.7071f,    0.0f), Anglef(-90.f,   0.f,   0.f));
	
	addTestData(glm::quat(0.7071f,    0.0f,    0.0f, 0.7071f), Anglef(  0.f,  90.f,   0.f));
	addTestData(glm::quat(   0.5f,    0.5f,    0.5f,    0.5f), Anglef( 90.f,  90.f,   0.f));
	addTestData(glm::quat(   0.0f, 0.7071f, 0.7071f,    0.0f), Anglef(180.f,  90.f,   0.f));
	addTestData(glm::quat(   0.5f,   -0.5f,   -0.5f,    0.5f), Anglef(-90.f,  90.f,   0.f));
	
	addTestData(glm::quat(0.7071f,    0.0f,    0.0f,-0.7071f), Anglef(  0.f, -90.f,   0.f));
	addTestData(glm::quat(   0.5f,   -0.5f,    0.5f,   -0.5f), Anglef( 90.f, -90.f,   0.f));
	addTestData(glm::quat(   0.0f,-0.7071f, 0.7071f,    0.0f), Anglef(180.f, -90.f,   0.f));
	addTestData(glm::quat(   0.5f,    0.5f,   -0.5f,   -0.5f), Anglef(-90.f, -90.f,   0.f));
	
	addTestData(glm::quat(0.7071f, 0.7071f,    0.0f,    0.0f), Anglef(  0.f,   0.f,  90.f));
	addTestData(glm::quat(   0.5f,    0.5f,    0.5f,   -0.5f), Anglef( 90.f,   0.f,  90.f));
	addTestData(glm::quat(   0.0f,    0.0f, 0.7071f,-0.7071f), Anglef(180.f,   0.f,  90.f));
	addTestData(glm::quat(   0.5f,    0.5f,   -0.5f,    0.5f), Anglef(-90.f,   0.f,  90.f));
	
	addTestData(glm::quat(   0.0f,    1.0f,    0.0f,    0.0f), Anglef(  0.f,   0.f, 180.f));
	addTestData(glm::quat(   0.0f, 0.7071f,    0.0f,-0.7071f), Anglef( 90.f,   0.f, 180.f));
	addTestData(glm::quat(   0.0f,    0.0f,    0.0f,    1.0f), Anglef(180.f,   0.f, 180.f));
	addTestData(glm::quat(   0.0f, 0.7071f,    0.0f, 0.7071f), Anglef(-90.f,   0.f, 180.f));
	
	addTestData(glm::quat(0.7071f,-0.7071f,    0.0f,    0.0f), Anglef(  0.f,   0.f, -90.f));
	addTestData(glm::quat(   0.5f,   -0.5f,    0.5f,    0.5f), Anglef( 90.f,   0.f, -90.f));
	addTestData(glm::quat(   0.0f,    0.0f, 0.7071f, 0.7071f), Anglef(180.f,   0.f, -90.f));
	addTestData(glm::quat(   0.5f,   -0.5f,   -0.5f,   -0.5f), Anglef(-90.f,   0.f, -90.f));
}

void LegacyMathTest::tearDown() {
	rotations.clear();
}

void LegacyMathTest::quaternionTests() {
	
	std::vector<TestRotation>::iterator it;
	for(it = rotations.begin(); it != rotations.end(); ++it) {
		
		glm::quat A = it->quat;	
		glm::quat B = it->quat;
	
		CPPUNIT_ASSERT_EQUAL(A, B);
		
		glm::quat inverseA = A;
		Quat_Reverse(&inverseA);
		glm::quat inverseB = glm::gtc::quaternion::inverse(B);
		
		CPPUNIT_ASSERT_EQUAL(inverseA, inverseB);
	
		Vec3f vecA = TransformVertexQuat(A, Vec3f(1.f, 0.5f, 0.1f));
		Vec3f vecB = B * Vec3f(1.f, 0.5f, 0.1f);
		
		CPPUNIT_ASSERT_EQUAL(vecA, vecB);
		
		glm::mat4x4 matrixA;
		MatrixFromQuat(matrixA, A);
		
		glm::mat4x4 matrixB = glm::toMat4(B);
		
		CPPUNIT_ASSERT_EQUAL(matrixA, matrixB);
	}
}

void LegacyMathTest::quatMuliplyTest() {
	glm::quat A = glm::quat(0.f,  1.f, 0.f, 0.f);
	glm::quat B = glm::quat(0.f,  0.f, 1.f, 0.f);
	
	CPPUNIT_ASSERT_EQUAL(A * B, Quat_Multiply(A, B));
}

void LegacyMathTest::quatSlerpTest() {
	
	typedef std::vector<TestRotation>::iterator itr;
	
	for(itr iA = rotations.begin(); iA != rotations.end(); ++iA) {
		for(itr iB = rotations.begin(); iB != rotations.end(); ++iB) {
			
			glm::quat expected = glm::mix(iA->quat, iB->quat, 0.5f);
			glm::quat result = Quat_Slerp(iA->quat, iB->quat, 0.5f);
			
			CPPUNIT_ASSERT_EQUAL(expected, result);
		}
	}
}

void LegacyMathTest::quatTransformVectorTest() {
	
	typedef std::vector<TestRotation>::iterator Itr;
	
	for(Itr it = rotations.begin(); it != rotations.end(); ++it) {
		const Vec3f testVert(1.f, 0.5f, 0.1f);
		
		Vec3f vecA = TransformVertexQuat(it->quat, testVert);
		Vec3f vecB = it->quat * testVert;
		
		CPPUNIT_ASSERT_EQUAL(vecA, vecB);
		
		Vec3f vecC;
		TransformInverseVertexQuat(it->quat, testVert, vecC);
		
		Vec3f vecD = glm::inverse(it->quat) * testVert;
		
		CPPUNIT_ASSERT_EQUAL(vecC, vecD);
	}
}

void LegacyMathTest::angleTest()
{
	typedef std::vector<TestRotation>::iterator Itr;
	
	for(Itr it = rotations.begin(); it != rotations.end(); ++it) {
		Vec3f testVert(1.f, 0.5f, 0.1f);
		
		EERIE_TRANSFORM trans;
		trans.updateFromAngle(it->angle);
		
		Vec3f out, temp, temp2;
		YRotatePoint(&testVert, &temp, trans.ycos, trans.ysin);
		XRotatePoint(&temp, &temp2, trans.xcos, trans.xsin);
		ZRotatePoint(&temp2, &out, trans.zcos, trans.zsin);
		
		Vec3f vecA = out;
		Vec3f vecB = Vec3f(trans.worldToView * Vec4f(testVert, 1.f));
		
		CPPUNIT_ASSERT_EQUAL(vecA, vecB);
	}
}