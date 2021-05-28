echo "current list of netns"
ip netns list

echo "deleting all netns"
ip -all netns delete

echo "list of deletion"
ip netns list

