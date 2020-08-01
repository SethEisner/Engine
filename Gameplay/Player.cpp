#include "Player.h"
#include "../RenderManager/Camera.h"
#include "../Engine.h"
#include "../InputManager/InputManager.h"
#include "../Utilities/Utilities.h"
#include "../Collision/CollisionObject.h"
#include "../Collision/RigidBody.h"

static const DirectX::XMVECTOR up_vector =       DirectX::XMVectorSet( 0.0f, 1.0f,  0.0f, 0.0f);
static const DirectX::XMVECTOR left_vector =     DirectX::XMVectorSet(-1.0f, 0.0f,  0.0f, 0.0f);
static const DirectX::XMVECTOR right_vector =    DirectX::XMVectorSet( 1.0f, 0.0f,  0.0f, 0.0f);
static const DirectX::XMVECTOR forward_vector =  DirectX::XMVectorSet( 0.0f, 0.0f,  1.0f, 0.0f);
static const DirectX::XMVECTOR backward_vector = DirectX::XMVectorSet( 0.0f, 0.0f, -1.0f, 0.0f);


Player::Player(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale) : GameObject(pos, scale),
m_jump_velocity(4.0f), m_acceleration(20.0f), m_max_speed(3.5), m_max_speed_sqrd(m_max_speed* m_max_speed), m_airborne(false) {}

void Player::update(double duration) {
	/* direction vector is filled with 1,0,-1 for each cardinal direction depending on which key is pressed or not
			d : x = 1
			a : x = -1
			w : z = -1;
			s : z = 1;
		jump vector gets ' ' : y = 1; meaning we set y to 1 when space is pressed, x & z in jump vector are always 0;
		if space is pressed, we need to directly set an upwards velocity
		need some logic to make sure we only add that velocity if we are o9n the ground, could maybe cache that value?
			only do the raycast if space is pressed, not held. additionally, should store the in air value as true until we press space again
			where then we perform another raycast to determine if we are in the air or on the ground
		speed should only be ground speed (x,z magnitude) so we dont clip fall speed
	*/
	if (!m_velocity) m_velocity = &m_collision_object->m_body->m_velocity; // get the up to date velocity from the rigidbody
	using namespace DirectX;
	XMVECTOR direction = get_direction();
	XMVECTOR velocity = XMLoadFloat3(m_velocity);
	XMVECTOR position = XMLoadFloat3(&m_position);
	float ground_speed = XMVectorGetX(XMVector3LengthSq(velocity * DirectX::XMVectorSet(1.0f, 0.0f, 1.0f, 0.0f))); // remove the y component from the magnitude
	// cap the ground speed
	float delta_velocity = m_acceleration * duration;
	if (XMVectorGetX(XMVector3LengthSq(direction)) > 0.0f) { // we are trying to move in some direction and thus should accleerate
		velocity += direction * delta_velocity;
		if (ground_speed > m_max_speed_sqrd) { // if we have accelerated sideways for long enough we will reach the max speed we want and will have to clip it
			float scale_factor = m_max_speed / sqrt(ground_speed); // scale factor is calucated from the similar triangle with the hypotenous as the desired max speed
			velocity *= DirectX::XMVectorSet(scale_factor, 1.0f, scale_factor, 0.0f); // * scale_factor;
		}
	}
	else if (ground_speed > 0.01){ // we are not trying to move in some direction and need to decelerate, up to a point
		// get velocity vector in opposite direction (negate x and z direction)
		// multiply this opposite vector by the delta velocity
		XMVECTOR opposite = velocity * XMVectorSet(-1.0f, 0.0f, -1.0f, 0.0f);
		velocity += opposite * delta_velocity;
	}
	else { // our horizontal velocity is super small so we can set it to zero
		velocity *= up_vector; // cancel out all horizontal velocity  
	}
	// perform a jumo if we are 
	if (!airborne() && engine->input_manager->is_pressed(HASH("jump"))) { // if we are not airborne then we can jump
		velocity += up_vector * m_jump_velocity;
	}
	// wake up the rigid body if we move it
	if (m_collision_object && !m_collision_object->m_body->get_awake() && XMVectorGetX(XMVector3LengthSq(velocity)) > 0.0f) { 
		m_collision_object->m_body->set_awake(true);
	}
	// integrate the velocity
	position += velocity * duration;
	XMStoreFloat3(m_velocity, velocity);
	XMStoreFloat3(&m_position, position);

	if (m_camera) { // if we have a camera attached to this player, we need to set its updated position
		XMFLOAT3 camera_pos;
		XMStoreFloat3(&camera_pos, position + up_vector); // add one to the y direction for the camera position
		m_camera->set_position(camera_pos);
	}
	// need to update the rotation of the gameobject here
}
bool  Player::airborne() {
	// update the value of airborne
	return m_airborne;
}
void Player::add_camera(Camera* camera) {
	m_camera = camera;
	m_components = m_components | HAS_CAMERA;
}
Camera* Player::remove_camera() {
	Camera* ret = m_camera;
	m_camera = nullptr;
	m_components = m_components & ~HAS_CAMERA;
	return ret;
}
DirectX::XMVECTOR Player::get_direction() {
	/* direction vector is filled with 1, 0, -1 for each cardinal direction depending on which key is pressed or not
			left : x = 1
			right : x = -1
			forward : z = -1;
			backward : z = 1;
	*/
	using namespace DirectX;
	XMVECTOR direction = XMVectorZero();
	if (engine->input_manager->is_activated(HASH("left"))) {
		direction += left_vector;
	}
	if (engine->input_manager->is_activated(HASH("right"))) {
		direction += right_vector;
	}
	if (engine->input_manager->is_activated(HASH("forward"))) {
		direction += forward_vector;
	}
	if (engine->input_manager->is_activated(HASH("backward"))) {
		direction += backward_vector;
	}
	return direction;
}