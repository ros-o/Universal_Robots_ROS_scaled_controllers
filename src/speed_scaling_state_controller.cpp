// this is for emacs file handling -*- mode: c++; indent-tabs-mode: nil -*-

// -- BEGIN LICENSE BLOCK ----------------------------------------------
// -- END LICENSE BLOCK ------------------------------------------------

//----------------------------------------------------------------------
/*!\file
 *
 * \author  Felix Mauch mauch@fzi.de
 * \date    2019-04-17
 *
 */
//----------------------------------------------------------------------

#include "ur_controllers/speed_scaling_state_controller.h"

#include <pluginlib/class_list_macros.hpp>

namespace ur_controllers
{
bool SpeedScalingStateController::init(SpeedScalingInterface* hw, ros::NodeHandle& root_nh,
                                       ros::NodeHandle& controller_nh)
{
  // get all joint states from the hardware interface
  const std::vector<std::string>& sensor_names = hw->getNames();
  for (unsigned i = 0; i < sensor_names.size(); i++)
    ROS_DEBUG("Got sensor %s", sensor_names[i].c_str());

  // get publishing period
  if (!controller_nh.getParam("publish_rate", publish_rate_))
  {
    ROS_ERROR("Parameter 'publish_rate' not set");
    return false;
  }

  for (unsigned i = 0; i < sensor_names.size(); i++)
  {
    // sensor handle
    sensors_.push_back(hw->getHandle(sensor_names[i]));

    // realtime publisher
    RtPublisherPtr rt_pub(new realtime_tools::RealtimePublisher<std_msgs::Float64>(root_nh, sensor_names[i], 4));
    realtime_pubs_.push_back(rt_pub);
  }

  // Last published times
  last_publish_times_.resize(sensor_names.size());
  return true;
}

void SpeedScalingStateController ::starting(const ros::Time& time)
{
  // initialize time
  for (unsigned i = 0; i < last_publish_times_.size(); i++)
  {
    last_publish_times_[i] = time;
  }
}

void SpeedScalingStateController ::update(const ros::Time& time, const ros::Duration& /*period*/)
{
  // limit rate of publishing
  for (unsigned i = 0; i < realtime_pubs_.size(); i++)
  {
    if (publish_rate_ > 0.0 && last_publish_times_[i] + ros::Duration(1.0 / publish_rate_) < time)
    {
      // try to publish
      if (realtime_pubs_[i]->trylock())
      {
        // we're actually publishing, so increment time
        last_publish_times_[i] = last_publish_times_[i] + ros::Duration(1.0 / publish_rate_);

        // populate message
        // realtime_pubs_[i]->msg_.header.stamp = time;
        realtime_pubs_[i]->msg_.data = *sensors_[i].getScalingFactor();

        realtime_pubs_[i]->unlockAndPublish();
      }
    }
  }
}

void SpeedScalingStateController ::stopping(const ros::Time& /*time*/)
{
}
}  // namespace ur_controllers

PLUGINLIB_EXPORT_CLASS(ur_controllers::SpeedScalingStateController, controller_interface::ControllerBase)
