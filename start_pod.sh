
#!/bin/bash
mkdir -p ./podshared
podman pod create grout
podman run -d --rm --pod grout \
		--privileged \
		-v ./podshared/:/run:Z \
		-v /dev/hugepages:/dev/hugepages \
		-v /dev/vfio:/dev/vfio \
		grout grout -v

podman run --rm --pod grout -ti -v ./podshared/:/run:Z grout grcli
