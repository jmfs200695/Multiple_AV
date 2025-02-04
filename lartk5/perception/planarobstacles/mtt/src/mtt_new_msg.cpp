/**************************************************************************************************
 Software License Agreement (BSD License)

 Copyright (c) 2011-2013, LAR toolkit developers - University of Aveiro - http://lars.mec.ua.pt
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted
 provided that the following conditions are met:

  *Redistributions of source code must retain the above copyright notice, this list of
   conditions and the following disclaimer.
  *Redistributions in binary form must reproduce the above copyright notice, this list of
   conditions and the following disclaimer in the documentation and/or other materials provided
   with the distribution.
  *Neither the name of the University of Aveiro nor the names of its contributors may be used to
   endorse or promote products derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************************************/
/**
\file
\brief Main node source code for the mtt algorithm, handlers and loop
*/

#include <mtt/mtt.h>

bool new_data=false;
sensor_msgs::PointCloud2 pointData;

void PointsHandler(const sensor_msgs::PointCloud2& msg)
{
    cout<<"Received point cloud"<<endl;
	pointData=msg;
	new_data=true;
}

void PointCloud2ToData(sensor_msgs::PointCloud2& cloud,t_data& data)//this function will convert the point cloud data into a laser scan type structure
{
	pcl::PointCloud<pcl::PointXYZ> PclCloud;
	
	pcl::fromROSMsg(cloud,PclCloud);
	
	
	double theta;
	map<double,pair<uint,double> > grid;
	map<double,pair<uint,double> >::iterator it;
	
	double spacing = 0.5*M_PI/180.;
	
	double rightBorder;
	double leftBorder;
	
	double r;
	
	cout<<"mtt Input size:"<<PclCloud.points.size()<<endl;
	//get points into grid to reorder them
	for(uint i=0;i<PclCloud.points.size();i++)
	{
		theta=atan2(PclCloud.points[i].y,PclCloud.points[i].x);
		r=sqrt(pow(PclCloud.points[i].x,2)+pow(PclCloud.points[i].y,2));
		
		//get closest theta, given a predefined precision
		rightBorder = spacing * ceil(theta/spacing);
		leftBorder = rightBorder - spacing;

		if(fabs(rightBorder-theta)<fabs(leftBorder-theta))
			theta=rightBorder;
		else
			theta=leftBorder;
		
		if(grid.find(theta)!=grid.end())
		{
			if(r<grid[theta].second)//using closest measurement
			{
				grid[theta].first=i;
				grid[theta].second=r;
			}
		}else
		{
			grid[theta].first=i;
			grid[theta].second=r;
		}
	}
	
	uint i=0;
	for(it=grid.begin();it!=grid.end();it++)
	{
		//the map auto orders the theta values
		data.x[i]=PclCloud.points[(*it).second.first].x;
		data.y[i]=PclCloud.points[(*it).second.first].y;
		data.r[i]=sqrt(pow(data.x[i],2)+pow(data.y[i],2));
		data.t[i]=atan2(data.y[i],data.x[i]);
		
		i++;
	}
	
	data.n_points=grid.size();
	cout<<"mtt Size of data:"<<data.n_points<<endl;
}




void CreateMarkers(vector<visualization_msgs::Marker>& marker_vector,mtt::TargetList& target_msg,vector<t_listPtr>& list)
{
	static map<pair<string,int>, pair<visualization_msgs::Marker,int> > marker_map;
	map<pair<string,int>, pair<visualization_msgs::Marker,int> >::iterator it;
	
	
	//limpar o vector todo
	marker_vector.clear();
	//colocar todos os elementos em processo de elemincação
	for(it=marker_map.begin();it!=marker_map.end();it++)
		it->second.second--;
	
	
	std_msgs::ColorRGBA color;
	class_colormap colormap("hsv",10, 1, false);
	
	visualization_msgs::Marker marker;
	
	marker.header.frame_id = pointData.header.frame_id;//target_msg.header.frame_id;
// 	cout<<"fid:"<<target_msg.header.frame_id<<endl;

	marker.header.stamp = ros::Time::now();
	marker.ns = "ids";
	marker.action = visualization_msgs::Marker::ADD;
	
	marker.pose.position.z=0.3;
	
	marker.type = visualization_msgs::Marker::TEXT_VIEW_FACING;
	
	marker.scale.x = 1.0; 
	marker.scale.y = 1.0; 
	marker.scale.z = 1.0; 

	marker.color.r=0;
	marker.color.g=0;
	marker.color.b=0;
	marker.color.a=1;
	
	marker.id=0;
	
	for(uint i=0;i<list.size();i++)
	{
		if(list[i]->shape.lines.size()!=0)
		{
			marker.pose.position.x=list[i]->position.estimated_x;
			marker.pose.position.y=list[i]->position.estimated_y;
			
			marker.text = boost::lexical_cast<string>(list[i]->id);
		
			marker.id++;
			
			marker_map[make_pair(marker.ns,marker.id) ] = make_pair(marker,1);//isto substitui ou cria o novo marker no map
		}
	}
	
    marker.pose.position.x=0;
    marker.pose.position.y=0;
    
    marker.text = "origin";

    marker.id++;
    
    marker_map[make_pair(marker.ns,marker.id) ] = make_pair(marker,1);//isto substitui ou cria o novo marker no map
        
	
	// Markers for Line objects 
	marker.type = visualization_msgs::Marker::LINE_STRIP;
	marker.ns = "objects";
	
	marker.pose.position.x=0;
	marker.pose.position.y=0;
	marker.pose.position.z=0;
	
	marker.scale.x = 0.2; 
	marker.scale.y = 0.1; 
	marker.scale.z = 0.1; 
	
	for(uint i=0;i<list.size();i++)
	{
		marker.color = colormap.color(list[i]->id);

		geometry_msgs::Point p;
		p.z = 0.1;
	
		marker.points.clear();
	
		uint l;
		for(l=0;l<list[i]->shape.lines.size();l++)
		{
			p.x = list[i]->shape.lines[l]->xi;
			p.y = list[i]->shape.lines[l]->yi;
			
			marker.points.push_back(p);
		}
	
		p.x = list[i]->shape.lines[l-1]->xf;
		p.y = list[i]->shape.lines[l-1]->yf;
		
		marker.points.push_back(p);
		
		marker.id++;
		
		marker_map[make_pair(marker.ns,marker.id) ] = make_pair(marker,1);//isto substitui ou cria o novo marker no map
	}
	
	
// 	// trying to create markers for vectors
//     marker.type = visualization_msgs::Marker::ARROW;
//     marker.ns = "velocities";
//     
//     marker.pose.position.x=0;
//     marker.pose.position.y=0;
//     marker.pose.position.z=0;
//     
//     marker.scale.x = 1; 
//     marker.scale.y = 1; 
//     marker.scale.z = 1; 
//     
//     for(uint i=0;i<list.size();i++)
//     {
//             marker.color = colormap.color(list[i]->id);      
//             marker.points.clear();
//             
//             marker.points[0].x = list[i]->position.estimated_x;
//             marker.points[0].y = list[i]->position.estimated_y;
//             marker.points[0].z = 0;
// 
//             marker.points[1].x = list[i]->position.estimated_x + list[i]->velocity.velocity_x;
//             marker.points[1].y = list[i]->position.estimated_y + list[i]->velocity.velocity_y;
//             marker.points[1].z = 0;
//             
//             marker.id++;
//             
//             marker_map[make_pair(marker.ns,marker.id) ] = make_pair(marker,1);//isto substitui ou cria o novo marker no map
//     }

	
	//para o map todo envio tudo, e meto tudo a false
	//envio todo e tudo o que ainda estiver a false vai com operaração de delete
	for(it=marker_map.begin();it!=marker_map.end();it++)
	{
		if( it->second.second==0 )//se for falso é para apagar
			it->second.first.action = visualization_msgs::Marker::DELETE;
		else if(it->second.second<=-1)//já foi apagado
		{
			/**
			\todo This should be erased but it is not working now
 			marker_map.erase(it);
			*/
			continue;
		}
		
		marker_vector.push_back(it->second.first);
	}
}

int main(int argc,char**argv)
{
	mtt::TargetList targetList;
	
	t_config	config;	
	t_data		full_data;
	t_flag		flags;
	
	vector<t_clustersPtr> clusters;
	vector<t_objectPtr> object;
	vector<t_listPtr> list;
	
	visualization_msgs::MarkerArray markersMsg;
	
	// Initialize ROS
	init(argc,argv,"mtt");
	
	NodeHandle nh("~");
	
	Subscriber subs_points = nh.subscribe("/points", 1000, PointsHandler);
	Publisher target_publisher = nh.advertise<mtt::TargetList>("/new_targets", 1000);
	Publisher markers_publisher= nh.advertise<visualization_msgs::MarkerArray>("/new_markers", 1000);
//         Publisher arrow_publisher= nh.advertise<visualization_msgs::MarkerArray>("/arrows", 1000);
// 	Publisher marker_publisher= nh.advertise<visualization_msgs::Marker>("/marker", 1000);
	
	init_flags(&flags);		//Inits flags values
	init_config(&config);	//Inits configuration values
	
	cout<<"Start to spin"<<endl;
	Rate r(100);
	while(ok())
	{
		spinOnce();
		r.sleep();
		
		if(!new_data)
			continue;
		new_data=false;
		
		//Get data from PointCloud2 to full_data
		PointCloud2ToData(pointData,full_data);
		
		//clustering		
		clustering(full_data,clusters,&config,&flags);
		cout<<"clusters: "<<clusters.size()<<endl;
		//calc_cluster_props
		calc_cluster_props(clusters,full_data);
		
		//clusters2objects		
		clusters2objects(object,clusters,full_data,config);
		
		calc_object_props(object);
		
		//AssociateObjects
		AssociateObjects(list,object,config,flags);
		
		//MotionModelsIteration
		MotionModelsIteration(list,config);

// 		cout<<"Number of targets "<<list.size()<<endl;
		
		clean_objets(object);//clean current objects
		
                //clear target list array
		targetList.Targets.clear();
                
        //structure to be fed to array
        mtt::Target target;

        //build header
        target.header.stamp = ros::Time::now();
        target.header.frame_id = pointData.header.frame_id;
		
        

        for(uint i=0;i<list.size();i++)
        {
                  geometry_msgs::Pose pose;
                  geometry_msgs::Twist vel;
                  geometry_msgs::Point initialpose;
                  geometry_msgs::Point finalpose;
          
                  //header
                  target.header.seq = list[i]->id;
                  target.id = list[i]->id;
                  
                  //velocity
                  vel.linear.x=list[i]->velocity.velocity_x;
                  vel.linear.y=list[i]->velocity.velocity_y;
                  vel.linear.z=0;
                  target.velocity = vel;
            
                  //compute orientation based on velocity.
                  double theta = atan2(vel.linear.y,vel.linear.x);
                  geometry_msgs::Quaternion target_orientation = 
                    tf::createQuaternionMsgFromYaw(theta);
                         
                  //pose
                  pose.position.x = list[i]->position.estimated_x;
                  pose.position.y = list[i]->position.estimated_y;
                  pose.position.z = 0;
                  pose.orientation = target_orientation;
                  target.pose = pose;
          
          //initialpose
          initialpose.x = list[i]->shape.lines[0]->xi;
          initialpose.y = list[i]->shape.lines[0]->yi;
          initialpose.z = 0;
          target.initialpose = initialpose;
          
          //finalpose
          finalpose.x = list[i]->shape.lines[list[i]->shape.lines.size()-1]->xf;
          finalpose.y = list[i]->shape.lines[list[i]->shape.lines.size()-1]->yf;
          finalpose.z = 0;
          target.finalpose = finalpose;
                      
          //target size
          target.size = sqrt(((finalpose.x - initialpose.x) * (finalpose.x - initialpose.x)) 
                    + ((finalpose.y - initialpose.y) * (finalpose.y - initialpose.y))); 
          
                  //feed array with current target
                  //condition to accept as valit target (procopio change)
                  double velocity = sqrt(pow(vel.linear.x,2)+
                                         pow(vel.linear.y,2));
                  
                  if(/*velocity > 0.5 &&*/ velocity < 3.0)
                  targetList.Targets.push_back(target);
                  
        }
                
        

        target_publisher.publish(targetList);
		
		CreateMarkers(markersMsg.markers,targetList,list);
		markers_publisher.publish(markersMsg);
//         arrow_publisher.publish(arrow_arrray);
		
		flags.fi=false;
	}
	
	return 0;
}
