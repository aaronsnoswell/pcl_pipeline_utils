
#include <ros/ros.h>
#include <sensor_msgs/PointCloud2.h>

#include <pcl_conversions/pcl_conversions.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/surface/mls.h>

#include "ros_msg_convert.h"


/**
 * Simple class to allow appling a Moving Least Squares smoothing filter
 */
class MovingLeastSquares {
private:
    double _search_radius;

public:
    MovingLeastSquares(double search_radius = 0.03)
        : _search_radius(search_radius)
    {
        // Pass
    };

    ros::Subscriber sub;
    ros::Publisher pub;
    void cloudCallback(const sensor_msgs::PointCloud2ConstPtr& cloud_msg);
};


/**
 * Callback that performs the Point Cloud downsapling
 */
void MovingLeastSquares::cloudCallback(const sensor_msgs::PointCloud2ConstPtr& msg)
{
    // Create a KD-Tree
    pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);

    // Output has the PointNormal type in order to store the normals calculated by MLS
    pcl::PointCloud<pcl::PointNormal> mls_points;

    // Init object (second point type is for the normals, even if unused)
    pcl::MovingLeastSquares<pcl::PointXYZ, pcl::PointNormal> mls;
    
    // Set parameters
    mls.setComputeNormals (true);
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud = ros_msg_convert::fromROSMsg(msg);
    mls.setInputCloud(cloud);
    mls.setPolynomialFit(true);
    mls.setSearchMethod(tree);
    mls.setSearchRadius(_search_radius);

    // Reconstruct
    mls.process(mls_points);

    // Output
    sensor_msgs::PointCloud2::Ptr out_cloud = ros_msg_convert::toROSMsg(mls_points);
    pub.publish(out_cloud);
}


/**
 * Main
 */
int main (int argc, char** argv)
{
    // Initialize ROS
    ros::init (argc, argv, "pcl_mls");
    ros::NodeHandle nh("~");

    // Read optional leaf_size argument
    double search_radius = 0.03;
    if (nh.hasParam("search_radius"))
    {
        nh.getParam("search_radius", search_radius);
        ROS_INFO("Using %0.4f as search radius", search_radius);
    }

    // Create our filter
    MovingLeastSquares MovingLeastSquaresObj(search_radius);
    const boost::function< void(const sensor_msgs::PointCloud2ConstPtr &)> boundCloudCallback = \
        boost::bind(&MovingLeastSquares::cloudCallback, &MovingLeastSquaresObj, _1);

    // Create a ROS subscriber for the input point cloud
    MovingLeastSquaresObj.sub = nh.subscribe<sensor_msgs::PointCloud2> ("/input", 10, boundCloudCallback);

    // Create a ROS publisher for the output point cloud
    MovingLeastSquaresObj.pub = nh.advertise<sensor_msgs::PointCloud2> ("/output", 10);

    // Spin
    ros::spin ();
}