#ifndef _PARTICLESYSTEM_H_
#define _PARTICLESYSTEM_H_

#include "../Header/3DLibs.h"
#include <vector>

//Get AVX instrinsics
#include <immintrin.h>


#include "../Header/Timer.h" 

#include "../Header/ThreadPool.h"

#define NUM_THREADS 4
#define WORKLOAD NUM_PARTICLES / NUM_THREADS
#define NUM_VERTEX_BUFFERS 3


class ParticleSystem
{
	private:
		float*		mXPosition;
		float*		mYPosition;
		float*		mZPosition;

		float*		mXVelocity;
		float*		mYVelocity;
		float*		mZVelocity;

		float*		mInitialXVelocity;
		float*		mInitialYVelocity;
		float*		mInitialZVelocity;

		ID3D11Buffer** mVertexBuffers;

		ID3D11Buffer*	mPerObjectCBuffer;
		PerObjectData	mPerObjectData;

		Timer mTimer;

		std::vector<std::future<void>> mResults;
		ThreadPool* mThreadPool;
		float		mDeltaTime;

	private:
		HRESULT		BuildMultipleVertexBuffer( ID3D11Device* device );
		HRESULT		UpdateMultipleVertexBuffer( ID3D11DeviceContext* deviceContext );
		HRESULT		UpdateAndSetPerObjectData( ID3D11DeviceContext* deviceContext );
		void		UpdateParticleLogic( float deltaTime, unsigned int beginAddress, unsigned int endAddress );
		void		SetRandomParticleVelocity( int index );
		void		CheckDeadParticles( unsigned int beginIndex, unsigned int endIndex );
		void		ResetParticle( int index );

	public:
		void Update( float deltaTime );
		void Render( ID3D11DeviceContext* deviceContext );

		HRESULT Initialize( ID3D11Device* device, ID3D11DeviceContext* deviceContext, XMFLOAT3 emitterPosition );
		void Release();

		ParticleSystem();
		~ParticleSystem();
};
#endif