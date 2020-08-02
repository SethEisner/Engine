#pragma once
#include "GameObject.h"

class Camera;
class GameObject;

// player is an extension of a gameobject with a Camera, and a special update function
struct Player : public GameObject {
	Camera* m_camera;
	virtual void update(double duration);
	Player() = delete;
	void add_camera(Camera* camera);
	Camera* remove_camera();
	Player(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f });
	bool airborne();
	//virtual void calculate_transform(); // replace GameObject calculate transform function because we only want to rotate about the y axis
private:
	DirectX::XMVECTOR get_direction();
	DirectX::XMFLOAT3* m_velocity = nullptr; // pointer to the velocity the rigidbody uses for integration
	float m_jump_velocity;
	float m_acceleration; // movement acceleration
	float m_max_speed; // max movement speed
	float m_max_speed_sqrd;
	bool m_airborne; 
	// implement once we have a remove from BVH function
	// bool m_perform_collision; // flag to determine if we perform collision (allow no-clip), if this is true we remove the collision object from the BVH and delete the node
};