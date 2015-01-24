/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2011, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

/* Author: Ioan Sucan */

#include "moveit_ompl_planning_interface/ConstrainedSampler.h"
#include <moveit/profiler/profiler.h>

moveit_ompl_interface::ConstrainedSampler::ConstrainedSampler(const OMPLPlanningContext *pc, const constraint_samplers::ConstraintSamplerPtr &cs)
  : ompl::base::StateSampler(pc->getOMPLStateSpace().get())
  , planning_context_(pc)
  , default_(space_->allocDefaultStateSampler())
  , constraint_sampler_(cs)
  , work_state_(pc->getCompleteInitialRobotState())
  , constrained_success_(0)
  , constrained_failure_(0)
{
  inv_dim_ = space_->getDimension() > 0 ? 1.0 / (double)space_->getDimension() : 1.0;
}

double moveit_ompl_interface::ConstrainedSampler::getConstrainedSamplingRate() const
{
  if (constrained_success_ == 0)
    return 0.0;
  else
    return (double)constrained_success_ / (double)(constrained_success_ + constrained_failure_);
}

bool moveit_ompl_interface::ConstrainedSampler::sampleC(ompl::base::State *state)
{
  //  moveit::Profiler::ScopedBlock sblock("sampleWithConstraints");

  //unsigned int max_attempts = planning_context_->getMaximumStateSamplingAttempts();
  unsigned int max_attempts = 4;

  if (constraint_sampler_->sample(work_state_, planning_context_->getCompleteInitialRobotState(), max_attempts))
  {
    planning_context_->getOMPLStateSpace()->copyToOMPLState(state, work_state_);
    if (space_->satisfiesBounds(state))
    {
      ++constrained_success_;
      return true;
    }
  }
  ++constrained_failure_;
  return false;
}

void moveit_ompl_interface::ConstrainedSampler::sampleUniform(ompl::base::State *state)
{
  if (!sampleC(state) && !sampleC(state) && !sampleC(state))
    default_->sampleUniform(state);
}

void moveit_ompl_interface::ConstrainedSampler::sampleUniformNear(ompl::base::State *state, const ompl::base::State *near, const double distance)
{
  if (sampleC(state) || sampleC(state) || sampleC(state))
  {
    double total_d = space_->distance(state, near);
    if (total_d > distance)
    {
      double dist = pow(rng_.uniform01(), inv_dim_) * distance;
      space_->interpolate(near, state, dist / total_d, state);
    }
  }
  else
    default_->sampleUniformNear(state, near, distance);
}

void moveit_ompl_interface::ConstrainedSampler::sampleGaussian(ompl::base::State *state, const ompl::base::State *mean, const double stdDev)
{
  if (sampleC(state) || sampleC(state) || sampleC(state))
  {
    double total_d = space_->distance(state, mean);
    double distance = rng_.gaussian(0.0, stdDev);
    if (total_d > distance)
    {
      double dist = pow(rng_.uniform01(), inv_dim_) * distance;
      space_->interpolate(mean, state, dist / total_d, state);
    }
  }
  else
    default_->sampleGaussian(state, mean, stdDev);
}