#ifndef TransformationPinning_h
#define TransformationPinning_h

class CTransformationPinning :
	public ITransformationPinning
{
public:
	struct TransformationPinJoint
	{
		enum Type
		{
			Copy		= 'C',
			Feather		= 'F',
			Inherit		= 'I'
		};
	};

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IAnimationPoseModifier)
		CRYINTERFACE_ADD(ITransformationPinning)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CTransformationPinning, "AnimationPoseModifier_TransformationPin", 0xcc34ddea972e47da, 0x93f9cdcb98c28c8e)

public:

public:
	virtual void SetBlendWeight(float factor);
	virtual void SetJoint(uint32 jntID);
	virtual void SetSource(ICharacterInstance* source);

	// IAnimationPoseModifier
public:
	virtual bool Prepare(const SAnimationPoseModifierParams& params) { return true; }
	virtual bool Execute(const SAnimationPoseModifierParams& params);
	virtual void Synchronize() { }

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
private:
	float m_factor;

	uint32 m_jointID;
	char *m_jointTypes;
	uint32 m_numJoints;
	ICharacterInstance* m_source;
	bool m_jointsInitialised;

	void Init(const SAnimationPoseModifierParams& params);

} _ALIGN(32);

#endif // TransformationPinning_h
