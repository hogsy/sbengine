#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

//----------------------------------------------------------------------
// CXR_Model_DavidTest
//----------------------------------------------------------------------

class CXR_Model_DavidTest : public CXR_Model_Custom
{

	MRTC_DECLARE;

protected:

	int m_iTexture;

private:

	//----------------------------------------------------------------------
	
	typedef	char*		InstanceID;

	//----------------------------------------------------------------------
	
	class Particle
	{
		public:

			BOOL			alive;

			CVec3Dfp32		position;
			CVec3Dfp32		velocity;
			CVec3Dfp32		acceleration;
			CVec3Dfp32		velocityfactor;
			fp32				angle;
			fp32				rotation;
			fp32				rotationchange;
			fp32				size;
			fp32				sizechange;
			int32			color;
			fp32				alpha;
			fp32				lifetime;
			fp32				lifetimeleft;

		public:

			Particle()
			{
				alive = FALSE;
				position = CVec3Dfp32(0, 0, 0);
				velocity = CVec3Dfp32(0, 0, 0);
				acceleration = CVec3Dfp32(0, 0, 0);
				velocityfactor = CVec3Dfp32(1, 1, 1);
				angle = 0;
				rotation = 0;
				rotationchange = 0;
				size = 1;
				sizechange = 0;
				color = 0xFFFFFFFF;
				lifetime = 0;
				lifetimeleft = 0;
			}

			BOOL
			isAlive()
			{
				return alive;
			}

			void
			spawn()
			{
				alive = TRUE;
				lifetimeleft = lifetime;
			}

			void
			kill()
			{
				alive = FALSE;
			}

			void
			update(fp32 deltatime)
			{
				lifetimeleft -= deltatime;

				if (lifetimeleft <= 0) {
					kill();
					return;
				}

				velocity.k[0] += velocity.k[0] * (velocityfactor.k[0] - 1.0f) * deltatime;
				velocity.k[1] += velocity.k[1] * (velocityfactor.k[1] - 1.0f) * deltatime;
				velocity.k[2] += velocity.k[2] * (velocityfactor.k[2] - 1.0f) * deltatime;
				velocity += acceleration * deltatime;
				position += velocity * deltatime;

				rotation += rotationchange * deltatime;
				angle += rotation * deltatime;

				size += sizechange * deltatime;

				alpha = lifetimeleft / lifetime;
//				alpha = 1.0f - ((1.0f - alpha));
				color = (color & 0x00808080) | ((int32)(alpha * 0x40) << 24);
			}

	};

	//----------------------------------------------------------------------

	class Instance : public CReferenceCount
	{

		private:

			enum { MaxNumParticles = 250 };

		private:

			fp32				time; // store animation time of last render call.
			fp32				size;
			fp32				duration;

			Particle*		particles;
			int32			numParticles;

static		int32			randseed;

		public:

			Instance(const CXR_AnimState* animstate)
			{
				duration = animstate->m_AnimTime1;
				size = animstate->m_Anim0;

				time = 0;

				particles = DNew(Particle) Particle[MaxNumParticles];
				numParticles = 0;

				addParticles(10);
			}

			void
			fillParticleArray(CXR_Particle2*& _particles, int32& _numParticles)
			{
				_numParticles = 0;
				_particles = DNew(CXR_Particle2) CXR_Particle2[numParticles];

				for (int32 i = 0; i < MaxNumParticles; i++) {
					if (particles[i].isAlive()) {
						_particles[_numParticles].m_Pos = particles[i].position;
						_particles[_numParticles].m_Angle = particles[i].angle;
						_particles[_numParticles].m_Size = particles[i].size;
						_particles[_numParticles].m_Color = particles[i].color;
						_numParticles++;
					}
				}
			}

			BOOL
			addParticle()
			{
				// Quick test for space.
				if (numParticles == MaxNumParticles)
					return FALSE;

				for (int32 i = 0; i < MaxNumParticles; i++) {
					if (!particles[i].isAlive()) {

						particles[i].position.k[0] = MFloat_GetRand(randseed++) - 0.5f;
						particles[i].position.k[1] = MFloat_GetRand(randseed++) - 0.5f;
						particles[i].position *= 5.0f;
						particles[i].position.k[2] = 0.5f + MFloat_GetRand(randseed++) * 0.5f;

						particles[i].velocity.k[0] = MFloat_GetRand(randseed++) - 0.5f;
						particles[i].velocity.k[1] = MFloat_GetRand(randseed++) - 0.5f;
						particles[i].velocity.Normalize();
						particles[i].velocity *= 5.0f + MFloat_GetRand(randseed++) * 30.0f;
//						particles[i].velocity.k[2] = MFloat_GetRand(randseed++) * 5.0f;

						particles[i].velocityfactor = CVec3Dfp32(-0.4f, -0.4f, 0.0f);
//						particles[i].acceleration = -particles[i].velocity * 0.01f;
//						particles[i].acceleration.k[2] = 10.0f;

						particles[i].size = 20;
						particles[i].sizechange = 50;

						particles[i].rotation = 0.25f;
						particles[i].rotationchange = -0.25f;

						particles[i].lifetime = duration * (0.4f + MFloat_GetRand(randseed++) * 0.6f);

						particles[i].spawn();

						numParticles++;

						return TRUE;
					}
				}

				return FALSE;
			}

			void
			addParticles(int32 numParticlesToAdd)
			{
				while (numParticlesToAdd > 0)
				{
					if (!addParticle())
						return;

					numParticlesToAdd--;
				}
			}

			void
			update(fp32 newtime)
			{
				fp32 deltatime = newtime - time;
				time = newtime;

				randseed += time;

				for (int32 i = 0; i < MaxNumParticles; i++) {
					particles[i].update(deltatime);
				}
			}
	};

	//----------------------------------------------------------------------
	
private:

	Instance*
	getCurrentInstance(const CXR_AnimState* animstate)
	{
		Instance* instance = (Instance*)(CReferenceCount*) *animstate->m_pspClientData;
		if (instance == NULL) {
			*animstate->m_pspClientData = DNew(Instance) Instance(animstate);
			if (*animstate->m_pspClientData == NULL) return NULL;
			instance = (Instance*)(CReferenceCount*) *animstate->m_pspClientData;
		}
		return instance;
	}
	
	//----------------------------------------------------------------------
	
	virtual void OnCreate(const char *)
	{
		m_iTexture = m_pTextureContext->GetTextureID("DustCloud1");
	}

	virtual bool RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
	{
//		_pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_NONE);
//		_pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
		_pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		_pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
		_pVB->m_pAttrib->Attrib_TextureID(0, m_iTexture);

		CXR_Particle2*		particles;
		int32				numParticles;

		Instance* instance = getCurrentInstance(_pAnimState);

		if (instance == NULL)
		{
			// Out of memory
			return false;
		}

		instance->update(_pAnimState->m_AnimTime0);
		instance->fillParticleArray(particles, numParticles);

		CMat43fp32 LocalToCamera;
		_WMat.Multiply(_VMat, LocalToCamera);

		// Man kan tydligen returnera resultatet fr≈n render rutinen direkt, troligen tom/icketom VB.
		return CXR_Util::Render_Particles(m_pVBM, _pVB, LocalToCamera, particles, numParticles, NULL, CXR_PARTICLETYPE_ANGLE | CXR_PARTICLETYPE_QUAD);
//		CXR_Util::Render_Particles(m_pVBM, _pVB, _WMat, _VMat, Beams, iBeam);

		return true;
	}
};

int32 CXR_Model_DavidTest::Instance::randseed = 0;

//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_DavidTest, CXR_Model_Custom);

//----------------------------------------------------------------------
	
