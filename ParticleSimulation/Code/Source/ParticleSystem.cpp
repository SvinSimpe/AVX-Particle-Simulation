#include "../Header/ParticleSystem.h"
#include <time.h>
#include <sstream>
#include <string>
#include <thread>

HRESULT ParticleSystem::BuildMultipleVertexBuffer( ID3D11Device* device )
{
	mVertexBuffers = new ID3D11Buffer*[NUM_VERTEX_BUFFERS];

	for ( size_t i = 0; i < NUM_VERTEX_BUFFERS; i++ )
	{
		//Build Vertex Buffers
		D3D11_BUFFER_DESC desc;
		ZeroMemory( &desc, sizeof( desc ) );

		desc.BindFlags				= D3D11_BIND_VERTEX_BUFFER;
		desc.Usage					= D3D11_USAGE_DYNAMIC;
		desc.ByteWidth				= sizeof(float) * NUM_PARTICLES;
		desc.StructureByteStride	= sizeof(float);
		desc.CPUAccessFlags			= D3D11_CPU_ACCESS_WRITE;

		if( FAILED( device->CreateBuffer( &desc, nullptr, &mVertexBuffers[i] ) ) )
			return E_FAIL;
	}

	return S_OK;
}

HRESULT ParticleSystem::UpdateMultipleVertexBuffer( ID3D11DeviceContext* deviceContext )
{

	// Update xPosition Vertex Buffer
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = deviceContext->Map( mVertexBuffers[0], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
	if( SUCCEEDED( hr ) )
	{
		memcpy( mappedResource.pData, &mXPosition[0], sizeof(float) * NUM_PARTICLES );
		deviceContext->Unmap( mVertexBuffers[0], 0 );
	}
	else 
		return E_FAIL;


	// Update yPosition Vertex Buffer
	hr = deviceContext->Map( mVertexBuffers[1], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
	if( SUCCEEDED( hr ) )
	{
		memcpy( mappedResource.pData, &mYPosition[0], sizeof(float) * NUM_PARTICLES );
		deviceContext->Unmap( mVertexBuffers[1], 0 );
	}
	else 
		return E_FAIL;


	//Update zPosition Vertex Buffer
	hr = deviceContext->Map( mVertexBuffers[2], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );
	if( SUCCEEDED( hr ) )
	{
		memcpy( mappedResource.pData, &mZPosition[0], sizeof(float) * NUM_PARTICLES );
		deviceContext->Unmap( mVertexBuffers[2], 0 );
	}
	else 
		return E_FAIL;


	return S_OK;
}

HRESULT ParticleSystem::UpdateAndSetPerObjectData( ID3D11DeviceContext* deviceContext )
{
	HRESULT hr = S_OK;

	XMStoreFloat4x4( &mPerObjectData.worldMatrix, XMMatrixTranspose( XMMatrixIdentity() ) );

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	hr = deviceContext->Map( mPerObjectCBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource );

	if( SUCCEEDED( hr ) )
	{
		memcpy( mappedResource.pData, &mPerObjectData, sizeof( mPerObjectData ) );	
		deviceContext->Unmap( mPerObjectCBuffer, 0 );

		deviceContext->GSSetConstantBuffers( 1, 1, &mPerObjectCBuffer );	
	}

	return hr;
}

void ParticleSystem::UpdateParticleLogic( float deltaTime, unsigned int beginAddress, unsigned int endAddress )
{
	const __m256 accelerationReg	= _mm256_set1_ps( -9.82f );
	const __m256 deltaReg			= _mm256_set1_ps( deltaTime );


	for ( unsigned int i = beginAddress; i < endAddress; i+=8 )
	{
		//Load positions into registers
		__m256 xPositionReg = _mm256_load_ps( &mXPosition[i] );
		__m256 yPositionReg = _mm256_load_ps( &mYPosition[i] );
		__m256 zPositionReg = _mm256_load_ps( &mZPosition[i] );

		//Load velocity into registers
		__m256 xVelocityReg = _mm256_load_ps( &mXVelocity[i] );
		__m256 yVelocityReg = _mm256_load_ps( &mYVelocity[i] );
		__m256 zVelocityReg = _mm256_load_ps( &mZVelocity[i] );

		// Calculate new Y-velocity based on acceleration and timestep (deltaTime)
		__m256 newYVelocityReg = _mm256_add_ps( _mm256_mul_ps( deltaReg , accelerationReg ),  yVelocityReg );

		// Store new Y-velocity
		yVelocityReg = newYVelocityReg;

		// Calculate new positions   |  oldPosition + ( velocity * timeStep )
		__m256 newXPositionReg = _mm256_add_ps( _mm256_mul_ps( deltaReg , xVelocityReg ), xPositionReg  );
		__m256 newYPositionReg = _mm256_add_ps( _mm256_mul_ps( deltaReg , yVelocityReg ), yPositionReg );
		__m256 newZPositionReg = _mm256_add_ps( _mm256_mul_ps( deltaReg , zVelocityReg ), zPositionReg );

		// Store new positions back to memory
		_mm256_store_ps( &mXPosition[i], newXPositionReg );
		_mm256_store_ps( &mYPosition[i], newYPositionReg );
		_mm256_store_ps( &mZPosition[i], newZPositionReg );

		// Store new velocity back to memory
		_mm256_store_ps( &mYVelocity[i], yVelocityReg );
	}
}

//Generate eight random velocities. The functions utilizes
//AVX-intrinsics for filling YMM-registers with random velocities and
//store into members for both Velocity and Initial Velocity
void ParticleSystem::SetRandomParticleVelocity( int index )
{
	XMFLOAT3 randomVelocity[8];
	float randomSpreadAngle = 0.0f;

	for ( int i = 0; i < 8; i++ )
	{
		randomSpreadAngle = (float)( ( rand() % 3000 * 2 ) - 3000 ) * 0.01f;
		XMVECTOR randomAimingDirection	= XMVector3TransformCoord( XMLoadFloat3( &XMFLOAT3( 0.0f, 1.0f, 0.0f ) ),
																	XMMatrixRotationX( XMConvertToRadians( randomSpreadAngle ) ) );
		
		float testMagnitude = (float)( ( rand() % 2500 ) + 1 ) * 0.001f;
		int magnitude = ( ( rand() % 30 ) + 38 );
		randomAimingDirection *= (magnitude + testMagnitude);

		XMStoreFloat3( &randomVelocity[i],
			XMVector3TransformCoord( randomAimingDirection,
				XMMatrixRotationZ( XMConvertToRadians( randomSpreadAngle ) ) ) );	
		
	}

	//Set random velocity to registers
	__m256 xVelReg = _mm256_set_ps( randomVelocity[0].x, randomVelocity[1].x,
									randomVelocity[2].x, randomVelocity[3].x,
									randomVelocity[4].x, randomVelocity[5].x,
									randomVelocity[6].x, randomVelocity[7].x );

	__m256 yVelReg = _mm256_set_ps( randomVelocity[0].y, randomVelocity[1].y,
			 						randomVelocity[2].y, randomVelocity[3].y,
			  						randomVelocity[4].y, randomVelocity[5].y,
									randomVelocity[6].y, randomVelocity[7].y );
	
	__m256 zVelReg = _mm256_set_ps( randomVelocity[0].z, randomVelocity[1].z,
			 						randomVelocity[2].z, randomVelocity[3].z,
			  						randomVelocity[4].z, randomVelocity[5].z,
									randomVelocity[6].z, randomVelocity[7].z );

	// Store random velocity in Velocity...
	_mm256_store_ps( &mXVelocity[index], xVelReg );
	_mm256_store_ps( &mYVelocity[index], yVelReg );
	_mm256_store_ps( &mZVelocity[index], zVelReg );

	// And in Initial Velocity
	_mm256_store_ps( &mInitialXVelocity[index], xVelReg );
	_mm256_store_ps( &mInitialYVelocity[index], yVelReg );
	_mm256_store_ps( &mInitialZVelocity[index], zVelReg );
}

void ParticleSystem::CheckDeadParticles( unsigned int beginIndex, unsigned int endIndex )
{
	//...............................................
	// Courtesy of user Paul R on stackoverflow.com 
	//...............................................
	const __m256 zeroReg	= _mm256_set1_ps( -200.0f ); 

	for ( unsigned int i = beginIndex; i + 8 <= endIndex; i += 8 )
	{
		__m256 yPositionReg	= _mm256_loadu_ps( &mYPosition[i] );					// load 8 x floats
		__m256 cmpReg		= _mm256_cmp_ps( yPositionReg, zeroReg, _CMP_LE_OS );	// compare for <= 0
		int	mask			= _mm256_movemask_ps( cmpReg );							// get MS bits from comparison result
		if ( mask != 0 )															// if any bits set
		{																			// then we have 1 or more elements <= 0
			for ( int k = 0; k < 8; ++k )											// test each element in vector
			{																		// using scalar code...
				if ( ( mask & 1 ) != 0 )
				{
					// found element at index i + k
					ResetParticle( i + k );
				}
				mask >>= 1;
			}
		}
	}

}

void ParticleSystem::ResetParticle( int index )
{
	mXPosition[index] = 0.0f;
	mYPosition[index] = 0.0f;
	mZPosition[index] = 0.0f;

	mXVelocity[index] = mInitialXVelocity[index];
	mYVelocity[index] = mInitialYVelocity[index];
	mZVelocity[index] = mInitialZVelocity[index];
}

void ParticleSystem::Update( float deltaTime )
{
	for ( size_t i = 0; i < NUM_THREADS; i++ )
	{
		// Check dead particles
		mResults.emplace_back( 
			mThreadPool->enqueue( [=] { CheckDeadParticles( i * WORKLOAD, WORKLOAD + ( i * WORKLOAD ) ); } ) );

		// Update particle logic
		mResults.emplace_back( 
			mThreadPool->enqueue( [=] { UpdateParticleLogic( deltaTime, i * WORKLOAD, WORKLOAD + ( i * WORKLOAD ) ); } ) );
	}
}

void ParticleSystem::Render( ID3D11DeviceContext* deviceContext )
{
	UpdateMultipleVertexBuffer( deviceContext );

	UpdateAndSetPerObjectData( deviceContext );

	deviceContext->Draw( NUM_PARTICLES, 0 );
}

HRESULT ParticleSystem::Initialize( ID3D11Device* device, ID3D11DeviceContext* deviceContext, XMFLOAT3 emitterPosition )
{
	//Allocating aligned memory for each of the attribute component. Allocated aligned
	//memory should be deallocated using '_aligned_free( void *memblock )'
	mXPosition = (float*) _aligned_malloc( NUM_PARTICLES * sizeof(float), 32 );
	mYPosition = (float*) _aligned_malloc( NUM_PARTICLES * sizeof(float), 32 );
	mZPosition = (float*) _aligned_malloc( NUM_PARTICLES * sizeof(float), 32 );

	mXVelocity = (float*) _aligned_malloc( NUM_PARTICLES * sizeof(float), 32 );
	mYVelocity = (float*) _aligned_malloc( NUM_PARTICLES * sizeof(float), 32 );
	mZVelocity = (float*) _aligned_malloc( NUM_PARTICLES * sizeof(float), 32 );

	mInitialXVelocity = (float*) _aligned_malloc( NUM_PARTICLES * sizeof(float), 32 );
	mInitialYVelocity = (float*) _aligned_malloc( NUM_PARTICLES * sizeof(float), 32 );
	mInitialZVelocity = (float*) _aligned_malloc( NUM_PARTICLES * sizeof(float), 32 );

	srand( (unsigned int)time(NULL));

	//Zeroes all YMM registers
	_mm256_zeroall();

	// Zero position members
	const __m256 zeroReg = _mm256_setzero_ps();

	//Since the AVX instructions used below is fetching 32bytes or a set of 8 single-precision
	//floating points from memory to registers, lets increment loop index with 8 per iteration
	for ( size_t i = 0; i < NUM_PARTICLES; i+=8 )
	{
		_mm256_store_ps( &mXPosition[i], zeroReg );
		_mm256_store_ps( &mYPosition[i], zeroReg );
		_mm256_store_ps( &mZPosition[i], zeroReg );

		SetRandomParticleVelocity( i );
	}
	
	if( FAILED( BuildMultipleVertexBuffer( device ) ) ) return E_FAIL;


	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth			= sizeof( PerObjectData );
	cbDesc.Usage				= D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags			= D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags		= D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags			= 0;
	cbDesc.StructureByteStride	= 0;


	if( FAILED( device->CreateBuffer( &cbDesc, nullptr, &mPerObjectCBuffer ) ) )
		return E_FAIL;


	mTimer.Initialize();
	mThreadPool = new ThreadPool( NUM_THREADS );


	//.......................Set Vertex Buffers..........................
	UINT32 offset[] = { 0, 0, 0 };
	UINT32 stride[]	= { sizeof(float), sizeof(float), sizeof(float) };
	deviceContext->IASetVertexBuffers( 0, 3, mVertexBuffers, stride, offset );
	//...................................................................
	
	return S_OK;
}

void ParticleSystem::Release()
{
	SAFE_RELEASE( mPerObjectCBuffer );
	  
	//Zeroes all YMM registers
	_mm256_zeroall();

	_aligned_free( mInitialZVelocity );
	_aligned_free( mInitialYVelocity );
	_aligned_free( mInitialXVelocity );

	_aligned_free( mZVelocity );
	_aligned_free( mYVelocity );
	_aligned_free( mXVelocity );

	_aligned_free( mZPosition );
	_aligned_free( mYPosition );
	_aligned_free( mXPosition );
}

ParticleSystem::ParticleSystem()
{
	mXPosition		= nullptr;
	mYPosition		= nullptr;
	mZPosition		= nullptr;
			  
	mXVelocity		= nullptr;
	mYVelocity		= nullptr;
	mZVelocity		= nullptr;

	mVertexBuffers		= nullptr;
	mPerObjectCBuffer	= nullptr;
	mThreadPool			= nullptr;
}

ParticleSystem::~ParticleSystem()
{}

